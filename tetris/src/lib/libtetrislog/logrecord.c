#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "logrecord.h"

/* ----- INTERNAL ----- */
// Sequence counter for tracking
static uint32_t next_seq = 0;

// Helper function that copies into a fixed char array and always terminates
// In the event len of source > len of dest then no \0 added
static void copy_fixed(char* dest, const char* src, size_t dest_size)
{
    if (src == NULL) // Invalid
    {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, dest_size - 1); // Copy up to dest_size - 1
    dest[dest_size - 1] = '\0';        // Add null terminator
}

/* ----- PACK / UNPACK ----- */
// Network Serialization
void pack_log_record(unsigned char buffer[LOG_WIRE_SIZE], const LogRecord* record)
{
    // Reset entire buffer before doing anything
    memset(buffer, 0, LOG_WIRE_SIZE);

    // Integers converted into network byte order before leaving
    uint32_t fields[5] = {
        htonl(record->time_sec),
        htonl(record->time_msec),
        htonl(record->level),
        htonl(record->pid),
        htonl(record->seq)};

    // Copy into buffer using manual offset
    size_t offset = 0;
    memcpy(buffer + offset, fields, sizeof(fields));
    offset += sizeof(fields);
    memcpy(buffer + offset, record->source, LOG_SOURCE_LENGTH);
    offset += LOG_SOURCE_LENGTH;
    memcpy(buffer + offset, record->message, LOG_MSG_LENGTH);
}

// Network Deserialization
void unpack_log_record(const unsigned char buffer[LOG_WIRE_SIZE], LogRecord* record)
{
    uint32_t fields[5]; // 5 fields to fill

    // Copy into buffer using manual offset
    size_t offset = 0;
    memcpy(fields, buffer + offset, sizeof(fields));
    offset += sizeof(fields);

    // Convert back into readable host integers
    record->time_sec = ntohl(fields[0]);
    record->time_msec = ntohl(fields[1]);
    record->level = ntohl(fields[2]);
    record->pid = ntohl(fields[3]);
    record->seq = ntohl(fields[4]);

    memcpy(record->source, buffer + offset, LOG_SOURCE_LENGTH);
    offset += LOG_SOURCE_LENGTH;
    memcpy(record->message, buffer + offset, LOG_MSG_LENGTH);

    // Extract the specified strings
    // Force last byte to \0 to prevent crashes
    record->source[LOG_SOURCE_LENGTH - 1] = '\0';
    record->message[LOG_MSG_LENGTH - 1] = '\0';
}

/* ----- HELPERS ----- */
// Record Builder
void build_log_record(LogRecord* record, LogLevel level, const char* source, const char* message)
{
    // Reset entire buffer before doing anything
    memset(record, 0, sizeof(LogRecord));

    // CLOCK_REALTIME -> actual system clock; ts to store output
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    // Fields; ts.tv_sec => seconds (s), ts.tv_nsec => nanoseconds (ns)
    record->time_sec = (uint32_t)ts.tv_sec;
    record->time_msec = (uint32_t)(ts.tv_nsec / 1000000); // ns -> ms
    record->level = (uint32_t)level;
    record->pid = (uint32_t)getpid();
    record->seq = next_seq++;

    copy_fixed(record->source, source, LOG_SOURCE_LENGTH);
    copy_fixed(record->message, message, LOG_MSG_LENGTH);
}

// Output Formatting
const char* log_level_name(LogLevel level)
{
    switch (level) {
    case LOG_LEVEL_DEBUG:
        return "DEBUG";
    case LOG_LEVEL_INFO:
        return "INFO";
    case LOG_LEVEL_WARN:
        return "WARN";
    case LOG_LEVEL_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN"; // Explicitly cannot return NULL pointer, so route to UNKNOWN
    }
}

// Output Formatting - 2
void format_log_line(const LogRecord* record, char* out, size_t out_size)
{
    // Convert epoch seconds into a readable local timestamp
    time_t raw = (time_t)record->time_sec;
    struct tm tm_buf;
    char timestamp[32] = {0};

    // localtime_r instead of localtime to be thread-safe (reentrant)
    if (localtime_r(&raw, &tm_buf) != NULL) {
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_buf);
    } else {
        // Unparseable time, keep the line aligned
        copy_fixed(timestamp, "0000-00-00 00:00:00", sizeof(timestamp));
    }

    // %-5s pads the level so the columns line up
    snprintf(out, out_size, "%s.%03u [%-5s] %s[%u] #%u %s\n",
             timestamp,
             record->time_msec,
             log_level_name((LogLevel)record->level),
             record->source,
             record->pid,
             record->seq,
             record->message);
}
