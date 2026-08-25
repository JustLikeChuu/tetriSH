#include <stdlib.h>
#include "targeting.h"

// Cycle target mode selection
void cycle_target_mode(GameState* player)
{
    player->target_mode = (player->target_mode + 1) % TARGET_MODE_COUNT;
}

// Cycle manual target selection
// Find current target in the lobby => move forward one at at time, wrap with total players
// Skips own player ID and anyone who is eliminated => lands on first valid hit
void cycle_manual_target(GameState* attacker, const Roster* roster)
{
    if (roster->count <= 1) // Nobody else in the lobby (or solo)
    {
        return;
    }

    // Finds the array index of the currently locked target -> resets to -1
    int current_index = -1;
    for (int i = 0; i < roster->count; i++) {
        if (roster->ids[i] == attacker->target_player_id) {
            current_index = i;
            break;
        }
    }

    // Loop through the roster step by step
    for (int step = 1; step <= roster->count; step++) {
        int index = (current_index + step + roster->count) % roster->count;

        // Assign iff it is a valid player
        if (roster->ids[index] != attacker->player_id && !roster->eliminated[index]) {
            attacker->target_player_id = roster->ids[index];
            return;
        }
    }
}

// Resolve and return correct target ID based on attacker's target mode
uint32_t resolve_target_id(GameState* attacker, const Roster* roster)
{
    switch (attacker->target_mode) {
    case TARGET_RANDOM: {
        uint32_t candidates[MAX_LOBBY_PLAYERS]; // To hold valid target IDs
        int count = 0;
        for (int i = 0; i < roster->count; i++) {
            // Add all living opponents into candidates array
            if (roster->ids[i] != attacker->player_id && !roster->eliminated[i]) {
                candidates[count++] = roster->ids[i];
            }
        }
        if (count == 0) // No valid candidates in array is given (caller only knows about itself)
        {
            return attacker->target_player_id; // Fallback to known / locked target
        }
        return candidates[rand() % count]; // Select random candidate from the pool
    }
    case TARGET_KO:                                                                               // Retaliation
        if (attacker->last_attacker_id != 0 && attacker->last_attacker_id != attacker->player_id) // Valid previous attacker that is not self
        {
            for (int i = 0; i < roster->count; i++) {
                // Verify iff previous attack is still in the game
                if (roster->ids[i] == attacker->last_attacker_id && !roster->eliminated[i]) {
                    return attacker->last_attacker_id;
                }
            }
        }
        // Else fallback to already locked target
        return attacker->target_player_id;
    case TARGET_MANUAL:
    default:
        // Use manually locked target selection
        return attacker->target_player_id;
    }
}
