#ifndef TETRISD_HUB_H
#define TETRISD_HUB_H

/*
 * Master <-> worker control channel!
 * 1 SOCK_SEQPACKET socketpair per room, carrying fixed-size HubMsg structs.
 * Everything that crosses a room boundary (joins, eliminations, cross-room garbage, etc.)
 * travels through here. See README!
 */
#include <stdint.h>
#include "lib/libtetrisprotocol/protocol.h"

// TODO: Move to tetrisrc
// Config
#define MAX_ROOMS 32
#define HUB_MAX_PLAYERS (MAX_ROOMS * MAX_LOBBY_SIZE)

// First TCP port handed to room workers
// Following room slot N will bind ROOM_PORT_BASE + N
#define ROOM_PORT_BASE 6710

// Same for UDP port, its own block so that TCP & UDP ranges can never collide
#define ROOM_UDP_PORT_BASE 6910

// Messages
typedef enum {
    HUB_READY,      // [worker -> master]: room port is bound, safe to redirect clients here!
    HUB_JOINED,     // [worker -> master]: a redirected client completed its join
    HUB_ATTACK,     // [worker -> master]: garbage aimed at a player in another room
    HUB_ELIMINATED, // [worker -> master]: a local player topped out or quit
    HUB_FEED,       // [worker -> master]: an attack happened, tell every other room's kill feed
                    // [master -> worker]: relayed attack, show it, do NOT apply garbage
    HUB_START,      // [master -> worker]: begin the match with everyone joined so far
    HUB_GARBAGE,    // [master -> worker]: apply garbage to one of this room's players
    HUB_ROSTER,     // [master -> worker]: refreshed global alive/dead view
    HUB_GAMEOVER    // [master -> worker]: global match ended, winner attached
} HubMsgType;

// One garbage attack crossing a room boundary
typedef struct
{
    uint32_t attacker_id;
    uint32_t victim_id;
    uint32_t lines;
} HubAttack;

// The master's view of every player in the match, alive or dead
typedef struct
{
    uint32_t count;
    uint32_t ids[HUB_MAX_PLAYERS];
    uint8_t alive[HUB_MAX_PLAYERS];
} HubRoster;

typedef struct
{
    uint32_t type; // HubMsgType
    union {
        uint32_t expected_players; // HUB_START
        uint32_t player_id;        // HUB_JOINED / HUB_ELIMINATED
        uint32_t winner_id;        // HUB_GAMEOVER
        HubAttack attack;          // HUB_ATTACK / HUB_GARBAGE / HUB_FEED
        HubRoster roster;          // HUB_ROSTER
    };
} HubMsg;

// SEQPACKET keeps message boundaries, so one send/recv moves one whole struct
int hub_send(int fd, const HubMsg* msg);

// Non-blocking: 1 = got a message, 0 = nothing waiting, -1 = peer closed/error
int hub_recv(int fd, HubMsg* msg);

#endif
