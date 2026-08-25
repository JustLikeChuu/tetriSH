#ifndef TETRISBRAIN_TARGETING_H
#define TETRISBRAIN_TARGETING_H

#include "state.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_LOBBY_PLAYERS 2 // TO BE KEPT IN SYNC

// Client POV, populated from server broadcast at game start -> also updated accordingly via elim broadcasts
typedef struct
{
    uint32_t ids[MAX_LOBBY_PLAYERS];    // Stores all 32-bit player IDs in the lobby
    bool eliminated[MAX_LOBBY_PLAYERS]; // Tracks life/death status of each player -> used to determine targeting
    int count;                          // Total num of active players in the lobby
} Roster;

// Cycle target mode selection
void cycle_target_mode(GameState* player);

// Cycle manual target selection
// Skips own player ID and anyone marked eliminated => lands on first valid hit
void cycle_manual_target(GameState* attacker, const Roster* roster);

// Resolve and return correct target ID based on attacker's targeting mode
uint32_t resolve_target_id(GameState* attacker, const Roster* roster);

#endif
