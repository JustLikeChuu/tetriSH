#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include "lib/libtetrisprotocol/protocol.h"
#include "lib/libtetrislog/logclient.h"
#include "hub.h"
#include "room.h"

// TODO: Move to tetrisrc
// Keep in sync with libbattleroyale's PORT_TCP
#define MASTER_PORT 6700

/* ----- MASTER STATE ----- */
// One per-room worker process, as the master sees it
typedef struct
{
    bool used;
    bool ready;   // Worker has bound its port, safe to redirect clients
    bool started; // Match is running in this room
    pid_t pid;
    int hub_fd;
    uint16_t port;
    int redirected;                  // Clients pointed at this room (but might have be joined yet)
    int joined;                      // Arrivals the worker has confirmed
    int pending_fds[MAX_LOBBY_SIZE]; // "Waiting Room", parked sockets until worker has initialised
    int pending_count;
} Room;

// One joined player anywhere in the match
typedef struct
{
    uint32_t id;
    int room; // Slot index, used to route garbage to the right worker
    bool alive;
} PlayerRec;

static LogClient log_client;
static Room rooms[MAX_ROOMS];
static PlayerRec players[HUB_MAX_PLAYERS];
static uint32_t player_count = 0;
static uint32_t room_seq = 0; // Grows forever so player ids never repeat
static int room_size = MAX_LOBBY_SIZE;
static int listen_fd = -1;
static bool any_room_started = false;
static bool match_ended = false; // Set once the real winner is decided and broadcast
static volatile sig_atomic_t stop_requested = 0;

static void on_sigint(int sig)
{
    (void)sig;
    stop_requested = 1; // Set flag, to cleanup on next iteration of main loop
}

static void log_message(LogLevel level, const char* fmt, ...)
{
    char message[LOG_MSG_LENGTH];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    printf("%s\n", message);
    log_client_push(&log_client, level, message); // Forwarded, non-blocking, may be dropped
}

/* ----- SETUP HELPERS ----- */
// Selects the first non-loopback IPv4 address, for display only for others to join
static void find_lan_ip(char host_ip[INET_ADDRSTRLEN])
{
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) != 0) {
        return;
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET || (ifa->ifa_flags & IFF_LOOPBACK)) {
            continue;
        }
        struct sockaddr_in* sa = (struct sockaddr_in*)ifa->ifa_addr;
        if (inet_ntop(AF_INET, &sa->sin_addr, host_ip, INET_ADDRSTRLEN) != NULL) {
            break; // First candidate found
        }
    }
    freeifaddrs(ifaddr);
}

static int master_listen(void)
{
    // Create a plain TCP socket, not yet bound to any port
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("[tetrisd] socket");
        return -1;
    }

    // Let a restarted master rebind MASTER_PORT immediately, skipping TIME_WAIT
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // Listen on every local interface, on the one well-known player-facing port
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(MASTER_PORT),
        .sin_addr.s_addr = INADDR_ANY};
    // Claim the port, then start queuing incoming connections (backlog 100)
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0 || listen(fd, 100) < 0) {
        perror("[tetrisd] bind/listen");
        close(fd);
        return -1;
    }
    return fd; // Ready for accept() in the main loop
}

/* ----- ROOMS ----- */
// Forks a fresh worker into a free slot; the worker binds its own port
static int spawn_room(void)
{
    int slot = -1;
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (!rooms[i].used) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        return -1; // Every slot has a live room
    }

    int sv[2];
    if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sv) < 0) {
        perror("[tetrisd] socketpair");
        return -1;
    }

    // A forever-growing base keeps ids unique even when slots are reused
    uint32_t id_base = 1 + room_seq * MAX_LOBBY_SIZE;
    uint16_t port = (uint16_t)(ROOM_PORT_BASE + slot);

    pid_t pid = fork();
    if (pid < 0) {
        perror("[tetrisd] fork");
        close(sv[0]);
        close(sv[1]);
        return -1;
    }
    if (pid == 0) {
        // Worker: shed every master-only fd, then run the room until it ends
        close(sv[0]);
        close(listen_fd);
        for (int i = 0; i < MAX_ROOMS; i++) {
            if (rooms[i].used) {
                close(rooms[i].hub_fd);
            }
        }
        room_worker_run(sv[1], port, id_base, slot);
        _exit(0); // Not reached, room_worker_run exits itself
    }

    close(sv[1]);
    memset(&rooms[slot], 0, sizeof(Room));
    rooms[slot].used = true;
    rooms[slot].pid = pid;
    rooms[slot].hub_fd = sv[0];
    rooms[slot].port = port;
    room_seq++;

    log_message(LOG_LEVEL_INFO, "[tetrisd] Room %d spawned (pid %d, port %u).", slot, pid, port);
    return slot;
}

// The room currently taking new players, spawning one if none is open
static int filling_room(void)
{
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].used && !rooms[i].started && rooms[i].redirected < room_size) {
            return i;
        }
    }
    return spawn_room();
}

// Redirection of clients to a worker: one port number, then the client reconnects to the worker
static void send_redirect(int client_fd, uint16_t port)
{
    uint32_t port_bytes = htonl(port);
    send(client_fd, &port_bytes, sizeof(port_bytes), MSG_NOSIGNAL); // Best effort
    close(client_fd);
}

static void accept_client(void)
{
    int client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd < 0) {
        return;
    }

    int slot = filling_room();
    if (slot < 0) {
        log_message(LOG_LEVEL_WARN, "[tetrisd] All %d rooms are full, refusing a connection.", MAX_ROOMS);
        close(client_fd);
        return;
    }

    Room* room = &rooms[slot];
    if (room->ready) {
        send_redirect(client_fd, room->port);
    } else if (room->pending_count < MAX_LOBBY_SIZE) {
        room->pending_fds[room->pending_count++] = client_fd; // Parked until HUB_READY
    } else {
        close(client_fd);
        return;
    }

    room->redirected++;
    log_message(LOG_LEVEL_INFO, "[tetrisd] Player dealt into room %d (%d/%d).", slot, room->redirected, room_size);
}

/* ----- GLOBAL MATCH TRACKING ----- */
// Pushes the master's alive/dead view to every running room
static void broadcast_roster(void)
{
    HubMsg msg = {.type = HUB_ROSTER};
    msg.roster.count = player_count;
    for (uint32_t i = 0; i < player_count && i < HUB_MAX_PLAYERS; i++) {
        msg.roster.ids[i] = players[i].id;
        msg.roster.alive[i] = players[i].alive ? 1 : 0;
    }

    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].used && rooms[i].started) {
            hub_send(rooms[i].hub_fd, &msg);
        }
    }
}

static void start_room(int slot)
{
    Room* room = &rooms[slot];
    if (!room->used || room->started || room->joined < 1) {
        return;
    }

    HubMsg msg = {.type = HUB_START, .expected_players = (uint32_t)room->redirected};
    hub_send(room->hub_fd, &msg);
    room->started = true;
    any_room_started = true;

    log_message(LOG_LEVEL_INFO, "[tetrisd] Room %d starting with %d player(s).", slot, room->joined);
    broadcast_roster(); // Fresh rooms need the current global view right away
}

// Starts every room that has anyone waiting in it. Rooms are purely a backend
// sharding detail, so this is the only way a match starts, either the whole
// daemon reaches capacity or the operator presses ENTER
static void start_all_rooms(void)
{
    for (int i = 0; i < MAX_ROOMS; i++) {
        start_room(i);
    }
}

// The match ends when one player stands across every room; solo runs until top out.
// Runs at most once: a room exiting normally after this already fired must not
// be mistaken for a crash and re-decide the outcome (see close_room)
static void check_global_end(void)
{
    if (match_ended || player_count == 0) {
        return;
    }

    uint32_t alive = 0;
    uint32_t last_alive_id = 0;
    for (uint32_t i = 0; i < player_count; i++) {
        if (players[i].alive) {
            alive++;
            last_alive_id = players[i].id;
        }
    }

    bool over = (player_count >= 2) ? (alive <= 1) : (alive == 0);
    if (!over) {
        return;
    }

    match_ended = true;
    uint32_t winner_id = (alive == 1) ? last_alive_id : 0;
    HubMsg msg = {.type = HUB_GAMEOVER, .winner_id = winner_id};
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].used) {
            hub_send(rooms[i].hub_fd, &msg);
        }
    }

    if (winner_id != 0) {
        printf("\n[tetrisd] MATCH OVER! Winner: P%u\n", winner_id);
    } else {
        printf("\n[tetrisd] MATCH OVER! No survivors.\n");
    }
}

// A closed hub means the room's worker exited, either:
// (1) It crashed mid-match (a real elimination event other rooms need to hear about)
// (2) It finished normally after the match already ended (no need to re-announce)
static void close_room(int slot)
{
    Room* room = &rooms[slot];
    close(room->hub_fd);
    for (int i = 0; i < room->pending_count; i++) {
        close(room->pending_fds[i]);
    }
    room->used = false;
    log_message(LOG_LEVEL_INFO, "[tetrisd] Room %d closed.", slot);

    if (match_ended) {
        return; // Normal post-match exit, no crash to react to
    }

    bool changed = false;
    for (uint32_t i = 0; i < player_count; i++) {
        if (players[i].room == slot && players[i].alive) {
            players[i].alive = false; // Crash isolation: a dead room only kills its own
            changed = true;
        }
    }

    if (changed) {
        broadcast_roster();
        check_global_end();
    }
}

/* ----- HUB TRAFFIC ----- */
static void handle_hub_msg(int slot, const HubMsg* msg)
{
    Room* room = &rooms[slot];

    switch (msg->type) {
    case HUB_READY:
        room->ready = true;
        // Flush everyone who connected before the worker's port was bound
        for (int i = 0; i < room->pending_count; i++) {
            send_redirect(room->pending_fds[i], room->port);
        }
        room->pending_count = 0;
        break;

    case HUB_JOINED:
        if (player_count < HUB_MAX_PLAYERS) {
            players[player_count++] = (PlayerRec){.id = msg->player_id, .room = slot, .alive = true};
        }
        room->joined++;
        log_message(LOG_LEVEL_INFO, "[tetrisd] P%u joined room %d. %u player(s) in the match.", msg->player_id, slot, player_count);
        broadcast_roster();

        // Upon reaching maximum players, start all rooms!
        if (player_count >= (uint32_t)(room_size * MAX_ROOMS)) {
            log_message(LOG_LEVEL_INFO, "[tetrisd] Daemon at capacity (%u players), starting all rooms.", player_count);
            start_all_rooms();
        }
        break;

    case HUB_ELIMINATED:
        for (uint32_t i = 0; i < player_count; i++) {
            if (players[i].id == msg->player_id) {
                players[i].alive = false;
            }
        }
        broadcast_roster();
        check_global_end();
        break;

    case HUB_ATTACK: {
        // Route garbage to whichever room holds the victim
        const HubAttack* attack = &msg->attack;
        for (uint32_t i = 0; i < player_count; i++) {
            if (players[i].id != attack->victim_id || !players[i].alive) {
                continue;
            }
            Room* target = &rooms[players[i].room];
            if (target->used && target->started) {
                HubMsg garbage = {.type = HUB_GARBAGE, .attack = *attack};
                hub_send(target->hub_fd, &garbage);
                log_message(LOG_LEVEL_INFO, " <!> [tetrisd] ROUTED: P%u (room %d) attacked P%u (room %d) with %u lines!", attack->attacker_id, slot, attack->victim_id, players[i].room, attack->lines);
            }
            break;
        }
        break; // Unknown or dead victims mean the damage is dropped
    }

    case HUB_FEED: {
        // Every room's kill feed should show every attack, not just its own,
        // so the match looks like one giant room from the outside. Only
        // fan-out here, never touch garbage: the actual damage already
        // landed via HUB_GARBAGE (or locally) before this arrived.
        HubMsg feed = {.type = HUB_FEED, .attack = msg->attack};
        for (int i = 0; i < MAX_ROOMS; i++) {
            if (i != slot && rooms[i].used && rooms[i].started) {
                hub_send(rooms[i].hub_fd, &feed);
            }
        }
        break;
    }

    default:
        break;
    }
}

/* ----- MAIN LOOP ----- */
int main(void)
{
    // Line-buffered so logs survive being piped to a file
    setvbuf(stdout, NULL, _IOLBF, 0);

    // Workers are reaped automatically, and dead sockets report errors instead
    // of killing the daemon with SIGPIPE
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, on_sigint);

    if (!log_client_init(&log_client, LOG_DEFAULT_IPC_PATH, "tetrisd")) {
        printf("[tetrisd] Warning: could not set up logging client, continuing without it.\n");
    }

    // Optional room size override for demos and tests
    const char* size_env = getenv("TETRISD_ROOM_SIZE");
    if (size_env != NULL) {
        int parsed = atoi(size_env);
        if (parsed >= 1 && parsed <= MAX_LOBBY_SIZE) {
            room_size = parsed;
        }
    }

    // Tell the host which IPv4 address to hand out to other players
    char host_ip[INET_ADDRSTRLEN] = {0};
    find_lan_ip(host_ip);
    if (host_ip[0] != '\0') {
        log_message(LOG_LEVEL_INFO, "[tetrisd] Players on this network should enter: %s", host_ip);
    } else {
        log_message(LOG_LEVEL_WARN, "[tetrisd] No non-loopback IPv4 found, local play only (127.0.0.1).");
    }
    printf("[tetrisd] Players on this machine can use the default 127.0.0.1\n");

    listen_fd = master_listen();
    if (listen_fd < 0) {
        log_message(LOG_LEVEL_ERROR, "[tetrisd] Could not create server!");
        return -1;
    }

    log_message(LOG_LEVEL_INFO, "[tetrisd] Master listening on port %d, dealing players into rooms of %d.", MASTER_PORT, room_size);
    printf("[tetrisd] Full rooms start automatically. Press ENTER to start partially filled rooms.\n");

    bool stdin_open = true;
    while (!stop_requested) {
        // Rebuilt every pass because rooms come and go
        struct pollfd pfds[2 + MAX_ROOMS];
        int room_of_pfd[2 + MAX_ROOMS];
        int nfds = 0;

        // Always poll the public port for new connections
        pfds[nfds++] = (struct pollfd){.fd = listen_fd, .events = POLLIN};
        // Poll stdin only until it closes (piped runs shouldn't spin on it)
        if (stdin_open) {
            pfds[nfds++] = (struct pollfd){.fd = STDIN_FILENO, .events = POLLIN};
        }
        // Everything from here on is a room hub fd, remember where they start
        int first_room_pfd = nfds;
        // One entry per live room, room_of_pfd maps a poll index back to its slot
        for (int i = 0; i < MAX_ROOMS; i++) {
            if (rooms[i].used) {
                room_of_pfd[nfds] = i;
                pfds[nfds++] = (struct pollfd){.fd = rooms[i].hub_fd, .events = POLLIN};
            }
        }

        if (poll(pfds, (nfds_t)nfds, 100) < 0) {
            continue; // Interrupted by a signal, loop re-checks stop_requested
        }

        // New connection on the public port
        if (pfds[0].revents & POLLIN) {
            accept_client();
        }

        // ENTER starts every waiting room that has at least one player
        if (stdin_open && (pfds[1].revents & POLLIN)) {
            char input[64];
            ssize_t got = read(STDIN_FILENO, input, sizeof(input));
            if (got <= 0) {
                stdin_open = false; // stdin closed (piped run), stop polling it
            } else if (memchr(input, '\n', (size_t)got) != NULL) {
                start_all_rooms();
            }
        }

        // Worker traffic: drain each active hub, close rooms whose worker ended
        for (int p = first_room_pfd; p < nfds; p++) {
            if (pfds[p].revents == 0) {
                continue;
            }
            int slot = room_of_pfd[p];
            HubMsg msg;
            int got;
            while ((got = hub_recv(rooms[slot].hub_fd, &msg)) == 1) {
                handle_hub_msg(slot, &msg);
            }
            if (got < 0) {
                close_room(slot);
            }
        }

        log_client_drain(&log_client, LOG_CLIENT_DRAIN_BATCH);

        // The daemon's job is done once a match ran and every room wound down
        if (any_room_started) {
            bool any_open = false;
            for (int i = 0; i < MAX_ROOMS; i++) {
                if (rooms[i].used) {
                    any_open = true;
                    break;
                }
            }
            if (!any_open) {
                break;
            }
        }
    }

    /* --- SHUTDOWN --- */
    // Closing the hubs is the shutdown signal,
    // workers treat EOF as game over
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (rooms[i].used) {
            close_room(i);
        }
    }
    close(listen_fd);

    log_message(LOG_LEVEL_INFO, "[tetrisd] Shutting down. %u log record(s) dropped this run.", log_client_get_dropped_count(&log_client));
    log_client_drain(&log_client, LOG_CLIENT_DRAIN_BATCH); // Send that very last line too
    log_client_close(&log_client);
    return 0;
}
