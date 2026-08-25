#ifndef TETRISLOG_LOGCLIENT_H
#define TETRISLOG_LOGCLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/un.h>
#include "logrecord.h"
#include "utils/queue.h" // corestack's generic ring buffer

// How many queued records being sent per call (at a time)
// Bounds how long one call can take even after a burst of events queued up between ticks
#define LOG_CLIENT_DRAIN_BATCH 16

// Reuses corestack's own queue macro to define a queue
DEFINE_QUEUE(LogRecord, LogRecord, 256)

typedef struct
{
    int sock;                       // Socket file descriptor
    struct sockaddr_un dest;        // Address of tetrislogd daemon
    char source[LOG_SOURCE_LENGTH]; // Source, name of the binary to be stamped onto every record automatically
    LogRecordQueue queue;           // Local ring buffer
    uint32_t dropped_queue_full;    // Records lost because the local ring buffer was full
    bool daemon_unreachable_warned; // Only print the "can't reach tetrislogd" warning once, not every send
} LogClient;

// Opens the socket and prepares the client
bool log_client_init(LogClient* client, const char* log_ipc_path, const char* source_name);

// Pushing (logging a message); non-blocking
void log_client_push(LogClient* client, LogLevel level, const char* message);

// Draining (once per game tick); sends up to max_records queued records as individual datagrams
void log_client_drain(LogClient* client, int max_records);

// Metrics; total records dropped locally because the ring buffer was full
uint32_t log_client_get_dropped_count(const LogClient* client);

// Teardown
void log_client_close(LogClient* client);

#endif
