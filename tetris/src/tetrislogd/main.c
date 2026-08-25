#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "lib/libtetrislog/logrecord.h"

/* ----- CONFIG DEFAULTS ----- */
#define DEFAULT_LOG_PATH "var/log/tetris.log" // Path to store generated log files
#define DEFAULT_LOG_IPC LOG_DEFAULT_IPC_PATH  // IPC path, shared with logclient.c
#define DEFAULT_RC_PATH "../.tetrishrc"       // TODO: replace with tetrish's real path once it exists

// For reading the .tetrishrc file
#define CONFIG_PATH_LENGTH 256
#define CONFIG_LINE_LENGTH 256

// State Table to track the last seen sequence number for each active PID
#define MAX_TRACKED_SENDERS 32
#define SUMMARY_INTERVAL_SEC 30

/* ----- SIGNAL STATE ----- */
static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t reopen_requested = 0;

// Exit gracefully
static void handle_sigterm(int sig) // SIGTERM -> termination
{
    (void)sig;
    running = 0;
}

//
static void handle_sighup(int sig) // SIGHUP -> signal hang up
{
    (void)sig;
    reopen_requested = 1; // reopen the same file again -> old file edited by linux due to running out of space
}

/* ----- GAP TRACKING ----- */
// To track last seen sequence counter for different pids
typedef struct
{
    uint32_t pid;
    uint32_t last_seq;
    bool seen; // False until this pid's first record arrives, so we don't treat "0" as a gap
} SenderTracker;

// Finds this pid's slot, create one if this is the first record seen from it
static SenderTracker* find_tracker(SenderTracker trackers[], int* count, uint32_t pid)
{
    for (int i = 0; i < *count; i++) {
        if (trackers[i].pid == pid) // Looking for matching pid
        {
            return &trackers[i]; // return if found match
        }
    }
    if (*count < MAX_TRACKED_SENDERS) // New packet that arrives from pid and -> check if there's enough slots
    {
        // Update table
        trackers[*count].pid = pid;
        trackers[*count].seen = false;
        (*count)++;
        return &trackers[*count - 1];
    }
    return NULL; // Table full, extremely unlikely, drop tracking for this sender rather than crash
}

/* ----- CONFIG ----- */
// Load configuration
static void load_config(const char* rc_path, char* log_path, size_t log_path_size,
                        char* log_ipc, size_t log_ipc_size)
{
    // Defaults first, config file (if present) overrides them below
    strncpy(log_path, DEFAULT_LOG_PATH, log_path_size - 1);
    log_path[log_path_size - 1] = '\0';
    strncpy(log_ipc, DEFAULT_LOG_IPC, log_ipc_size - 1);
    log_ipc[log_ipc_size - 1] = '\0';

    FILE* f = fopen(rc_path, "r");
    if (f == NULL) {
        return; // No .tetrishrc yet, defaults are fine, this is not an error
    }

    char line[CONFIG_LINE_LENGTH];
    while (fgets(line, sizeof(line), f) != NULL) {
        // Search for a # -> if found, replaced with \0
        char* hash = strchr(line, '#');
        if (hash != NULL) {
            *hash = '\0';
        }
        // Replace any \r \n with \0
        line[strcspn(line, "\r\n")] = '\0';

        // Manual split on '=' -> splits into key and value without having to malloc
        char* eq = strchr(line, '=');
        if (eq == NULL) // Blank or malformed line, skip it
        {
            continue;
        }
        *eq = '\0';
        char* key = line;
        char* value = eq + 1;

        // Trim leading/trailing whitespace off both sides
        while (*key == ' ' || *key == '\t') {
            key++;
        }
        char* key_end = key + strlen(key);
        while (key_end > key && (key_end[-1] == ' ' || key_end[-1] == '\t')) {
            *(--key_end) = '\0';
        }
        while (*value == ' ' || *value == '\t') {
            value++;
        }
        char* value_end = value + strlen(value);
        while (value_end > value && (value_end[-1] == ' ' || value_end[-1] == '\t')) {
            *(--value_end) = '\0';
        }

        // Extracting the data
        if (strcmp(key, "log_path") == 0) {
            strncpy(log_path, value, log_path_size - 1);
            log_path[log_path_size - 1] = '\0';
        } else if (strcmp(key, "log_ipc") == 0) {
            strncpy(log_ipc, value, log_ipc_size - 1);
            log_ipc[log_ipc_size - 1] = '\0';
        }
    }

    fclose(f);
}

/* ----- PATH SETUP ----- */
// Need to create the directories first; if they don't exist if not fopen() and bind() would not work
static void ensure_parent_dir_exists(const char* file_path)
{
    // Copies the path
    char path[CONFIG_PATH_LENGTH];
    strncpy(path, file_path, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';

    // Replace last / with \0 to cut the filename off the string
    char* last_slash = strrchr(path, '/');
    if (last_slash == NULL) {
        return; // No directory component, file is in the current directory
    }
    *last_slash = '\0'; // Cut the string down to just the directory part

    // Walk the path left to right, creating each level
    // Starting at path[1] skips a leading '/' for absolute paths
    for (char* p = path + 1; *p != '\0'; p++) {
        if (*p == '/') {
            *p = '\0';         // Temporary overwrite with \0
            mkdir(path, 0755); // Ignore the error, EEXIST is expected and fine
            *p = '/';          // Restore back the / so can continue down the path
        }
    }
    mkdir(path, 0755); // Final segment
}

/* ----- MAIN FUNCTION ----- */
int main(int argc, char* argv[])
{
    // Print line-by-line instantly, rather than buffering in large chunks
    setvbuf(stdout, NULL, _IOLBF, 0);

    // Setting paths + their lengths
    char log_path[CONFIG_PATH_LENGTH];
    char log_ipc[CONFIG_PATH_LENGTH];

    // Load the configuration desired
    load_config(DEFAULT_RC_PATH, log_path, sizeof(log_path), log_ipc, sizeof(log_ipc));

    // TO REMOVE: CLI flags override the config file, useful for testing without a .tetrishrc at all
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--log-path") == 0) {
            strncpy(log_path, argv[i + 1], sizeof(log_path) - 1);
            log_path[sizeof(log_path) - 1] = '\0';
        } else if (strcmp(argv[i], "--log-ipc") == 0) {
            strncpy(log_ipc, argv[i + 1], sizeof(log_ipc) - 1);
            log_ipc[sizeof(log_ipc) - 1] = '\0';
        }
    }

    // Kernel limit check -> limits socket paths to 108 bytes
    // Preventing silent truncation of long paths
    struct sockaddr_un check_size;
    if (strlen(log_ipc) >= sizeof(check_size.sun_path)) {
        fprintf(stderr, "[tetrislogd] log_ipc path too long: %s\n", log_ipc);
        return 1;
    }

    printf("[tetrislogd] log_path=%s\n", log_path);
    printf("[tetrislogd] log_ipc=%s\n", log_ipc);

    // Register the signals
    signal(SIGTERM, handle_sigterm);
    signal(SIGHUP, handle_sighup);
    signal(SIGPIPE, SIG_IGN); // Tells OS to not kill the daemon if client programs dies halfway while talking

    ensure_parent_dir_exists(log_path);
    ensure_parent_dir_exists(log_ipc);

    // Create the receiving socket
    int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[tetrislogd] Socket unable to be created!");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, log_ipc, sizeof(addr.sun_path) - 1);

    unlink(log_ipc); // Remove a stale socket file left behind by a previous run

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[tetrislogd] Binding failed!");
        close(sock);
        return 1;
    }

    // Open file in append mode
    FILE* log_file = fopen(log_path, "a");
    if (log_file == NULL) {
        perror("[tetrislogd] Unable to open log file!");
        close(sock);
        unlink(log_ipc);
        return 1;
    }

    SenderTracker trackers[MAX_TRACKED_SENDERS];
    int tracker_count = 0;
    uint32_t channel_dropped_total = 0;  // Gaps detected via seq, for the whole run
    uint32_t channel_dropped_period = 0; // Since the last periodic summary line
    time_t last_summary = time(NULL);

    printf("[tetrislogd] listening on %s, writing to %s\n", log_ipc, log_path);

    // Main Event Loop
    while (running) {
        // SIGHUP was requested by a signal handler, reopen
        if (reopen_requested) {
            fclose(log_file);
            log_file = fopen(log_path, "a");
            if (log_file == NULL) {
                perror("[tetrislogd] Logfile does not exist!");
                break;
            }
            reopen_requested = 0;
            fprintf(stderr, "[tetrislogd] log file reopened on SIGHUP\n");
        }

        // Async sleep with select() and 1s timeout
        // Wakes up exactly when a log arrives, or after 1 second (whichever comes first)
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        struct timeval tv = {1, 0};

        int activity = select(sock + 1, &fds, NULL, NULL, &tv);
        if (activity < 0) {
            if (errno == EINTR) {
                continue; // Interrupted by a signal, loop back around and recheck flags
            }
            perror("[tetrislogd] Select interrupted!");
            break;
        }

        // Reading and Writing
        if (activity > 0 && FD_ISSET(sock, &fds)) {
            // Read the raw bytes from the socket
            unsigned char buffer[LOG_WIRE_SIZE];
            ssize_t n = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);

            // Only take packets that are the correct size
            if (n == (ssize_t)LOG_WIRE_SIZE) {
                LogRecord record;
                unpack_log_record(buffer, &record);

                SenderTracker* t = find_tracker(trackers, &tracker_count, record.pid);
                if (t != NULL) {
                    if (t->seen && record.seq != t->last_seq + 1 && record.seq > t->last_seq) {
                        // Calculates number of dropped packets over a specified period by comparing seq number against the tracker
                        uint32_t gap = record.seq - t->last_seq - 1;
                        channel_dropped_total += gap;
                        channel_dropped_period += gap;
                    }
                    t->last_seq = record.seq;
                    t->seen = true;
                }

                // Convert into something readable
                char line[LOG_LINE_LENGTH];
                format_log_line(&record, line, sizeof(line));
                fputs(line, log_file);
                fflush(log_file); // Push output instantly, don't want to risk losing logs
            }
            // Ignore if datagram is the wrong size
        }

        time_t now = time(NULL);
        if (now - last_summary >= SUMMARY_INTERVAL_SEC) {
            if (channel_dropped_period > 0) {
                fprintf(stderr, "[tetrislogd] dropped %u records in last %ds\n",
                        channel_dropped_period, SUMMARY_INTERVAL_SEC);
            }
            channel_dropped_period = 0;
            last_summary = now;
        }
    }

    // When SIGTERM sets running = 0; clean up
    fprintf(stderr, "[tetrislogd] shutting down, total channel-drops observed: %u\n", channel_dropped_total);
    fflush(log_file);
    fclose(log_file);
    close(sock);
    unlink(log_ipc);
    return 0;
}
