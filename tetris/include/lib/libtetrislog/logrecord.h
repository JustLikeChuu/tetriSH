#ifndef TETRISLOG_LOGRECORD_H
#define TETRISLOG_LOGRECORD_H

#include <stdint.h>
#include <stddef.h>

/* ----- DEFAULT IPC PATH ----- */
#define LOG_DEFAULT_IPC_PATH "var/run/tetrislog.sock" // Default location to listen for incoming logs

/* ----- SIZES ----- */
#define LOG_SOURCE_LENGTH 16 // Log Source (bytes)
#define LOG_MSG_LENGTH 256   // Actual Message Text; max length, anything longer will be truncated (bytes)
#define LOG_LINE_LENGTH 384  // Max length of the entire string being written

/* ----- LEVELS ----- */
// Mirrors corestack's LOG_D / LOG_I / LOG_W / LOG_E macros
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

/* ----- RECORD ----- */
// One log entry, exactly one of these travels per datagram
typedef struct
{
    uint32_t time_sec;              // Timestamp (seconds since epoch)
    uint32_t time_msec;             // Milliseconds remainder (for easier sorting)
    uint32_t level;                 // LogLevel
    uint32_t pid;                   // Process ID -> to distinguish new instance
    uint32_t seq;                   // Sequence number -> increments with every log sent via same PID
    char source[LOG_SOURCE_LENGTH]; // Source
    char message[LOG_MSG_LENGTH];   // Log Payload
} LogRecord;

// Exact number of bytes on the wire, one datagram is always this size
// (5 * 4) + 16 + 256 = 292 bytes sent per datagram
#define LOG_WIRE_SIZE (5 * sizeof(uint32_t) + LOG_SOURCE_LENGTH + LOG_MSG_LENGTH)

/* ----- PACK / UNPACK -----*/
void pack_log_record(unsigned char buffer[LOG_WIRE_SIZE], const LogRecord* record);
void unpack_log_record(const unsigned char buffer[LOG_WIRE_SIZE], LogRecord* record);

/* ----- HELPERS ----- */
// Fills in time, pid and seq automatically, and truncates safely
// Callers only supply what they actually know: level, source, message
void build_log_record(LogRecord* record, LogLevel level, const char* source, const char* message);

// "DEBUG" / "INFO" / "WARN" / "ERROR"
const char* log_level_name(LogLevel level);

// Renders one record into the line written to disk
// Format: YYYY-MM-DD HH:MM:SS [INFO ] tetrisd[1234] #42 message text
void format_log_line(const LogRecord* record, char* out, size_t out_size);

#endif
