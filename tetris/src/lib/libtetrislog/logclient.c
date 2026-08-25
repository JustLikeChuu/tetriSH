#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "logclient.h"

/* ----- SETUP ----- */
// Initialization
bool log_client_init(LogClient* client, const char* log_ipc_path, const char* source_name)
{
    // Reset entire buffer before doing anything
    memset(client, 0, sizeof(LogClient));
    // Init the ring buffer
    LogRecord_init(&client->queue);

    strncpy(client->source, source_name, LOG_SOURCE_LENGTH - 1);
    client->source[LOG_SOURCE_LENGTH - 1] = '\0';

    client->sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (client->sock < 0) {
        perror("[logclient] Socket initialization failed!");
        return false;
    }

    // Add O_NONBLOCK flag to the socket -> by default is blocking
    // sendto() will freeze game server thread until space is freed up
    int flags = fcntl(client->sock, F_GETFL, 0);
    fcntl(client->sock, F_SETFL, flags | O_NONBLOCK); // If daemon is busy or dead, sendto() returns -1 instead of freezing

    // Pre-fills destination address
    memset(&client->dest, 0, sizeof(client->dest));
    client->dest.sun_family = AF_UNIX;
    strncpy(client->dest.sun_path, log_ipc_path, sizeof(client->dest.sun_path) - 1);

    return true;
}

/* ----- PUSH ----- */
void log_client_push(LogClient* client, LogLevel level, const char* message)
{
    if (client->sock < 0) {
        return; // Not initialized, nothing to queue
    }

    LogRecord record;
    build_log_record(&record, level, client->source, message); // Uses helper function to build the logs

    if (!LogRecord_enqueue(&client->queue, record)) {
        client->dropped_queue_full++; // Ring buffer full, this record never gets sent
    }
}

/* ----- DRAIN ----- */
void log_client_drain(LogClient* client, int max_records)
{
    if (client->sock < 0) {
        return; // Not initialized, nothing to dequeue
    }

    for (int i = 0; i < max_records; i++) {
        LogRecord* record = LogRecord_peek(&client->queue); // Check the next record in queue
        if (record == NULL) {
            break; // Nothing left queued this tick
        }

        // Serialize the struct and fires the datagram at the socket path
        unsigned char buffer[LOG_WIRE_SIZE];
        pack_log_record(buffer, record);

        ssize_t sent = sendto(client->sock, buffer, LOG_WIRE_SIZE, 0, (struct sockaddr*)&client->dest, sizeof(client->dest));

        if (sent < 0) {
            // Etiher tetrislogd is not running or its receive buffer is momentarily full
            // This specific record is lost
            if (!client->daemon_unreachable_warned) {
                fprintf(stderr, "[logclient] cannot reach tetrislogd at %s (further failures will be counted silently)\n", client->dest.sun_path);
                client->daemon_unreachable_warned = true;
            }
            client->dropped_queue_full++;
        } else {
            client->daemon_unreachable_warned = false; // Recovered, allow the warning to fire again if it drops later
        }

        LogRecord_dequeue(&client->queue); // Removed either way, sent or not
    }
}

/* ----- STATUS / TEARDOWN ----- */
uint32_t log_client_get_dropped_count(const LogClient* client)
{
    return client->dropped_queue_full;
}

void log_client_close(LogClient* client)
{
    // ENsures file descriptor is released back when game server shuts down
    if (client->sock >= 0) {
        close(client->sock);
        client->sock = -1;
    }
}
