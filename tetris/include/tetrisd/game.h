#ifndef TETRISD_GAME_H
#define TETRISD_GAME_H

#include <stdbool.h>
#include <stdint.h>
#include "lib/libtetrisprotocol/protocol.h"
#include "lib/libhtttp/payload.h"
#include "lib/libtetrisbrain/state.h"
#include "lib/libtetrisbrain/targeting.h"

/* ----- TIMING ----- */
// How often the authoritative simulation advances
#define TICK_MICROSECONDS 10000

// Resend a player's state after this many idle ticks even when nothing changed
#define KEEPALIVE_TICKS 50

// Upper bound on messages drained per tick, stops a flood of client packets from starving the simulation
#define MAX_MSGS_PER_TICK 64

/* ----- PLAYER SLOT ----- */
// One player's authoritative board
typedef struct
{
    GameState state;    // The real board, the client only ever renders a copy
    uint32_t player_id; // Server-assigned ID this slot belongs to
    bool active;        // Slot is in use
    bool dirty;         // Something changed since the last broadcast
    bool elim_reported; // Master has already been told this player is out
    int idle_ticks;     // Ticks since this player's state was last sent
} PlayerSlot;

/* ----- SESSION ----- */
// Everything the server knows about the running match
typedef struct
{
    PlayerSlot players[MAX_LOBBY_SIZE];
    int count;     // Number of active slots
    Roster roster; // Shared lobby view, used to resolve targeting server-side
} GameSession;

// Builds authoritative state for every connected player
void init_session(GameSession* session, const uint32_t* client_ids, uint32_t lobby_size);

// Finds a player's slot by ID, returns NULL if that ID is not in the match
PlayerSlot* find_player(GameSession* session, uint32_t player_id);

// Performs one requested action on that player's authoritative board
void apply_action(GameSession* session, PlayerSlot* slot, PlayerAction action);

// Advances gravity and lock delay for every active player by one tick
void tick_session(GameSession* session);

// Counts players who are still connected and not topped out
// Drives the match end condition in tetrisd's main loop
int count_alive_players(const GameSession* session);

#endif
