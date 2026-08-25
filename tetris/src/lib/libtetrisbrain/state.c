#include "state.h"
#include "bag.h"

#include <string.h>
#include <time.h>
#include <stdlib.h>

// Initialise gamestate
GameState gamestate_player;

// Function call to start the game
void start_game(GameState* state)
{
    // Reset game state
    memset(state, 0, sizeof(GameState));

    // Generate starting bag of pieces
    srand(time(NULL)); // Seed + current UNIX timestamp for pure randomization

    // Reset game variables
    state->held_type = 0;
    state->t_spins = 0;
    state->tetrises = 0;
    state->last_action_rotation = false;
    state->score = 0;
    state->game_over = 0;
    state->lines_cleared = 0;
    state->combo = -1;
    state->b2b = false;
    state->level = 1;
    state->pending_garbage = 0;
    state->outgoing_garbage = 0;
    state->winner_id = 0;

    // Initialize the 14-bag for preview pieces
    int bag1[7] = {1, 2, 3, 4, 5, 6, 7};
    int bag2[7] = {1, 2, 3, 4, 5, 6, 7};
    shuffle_array(bag1, 7);
    shuffle_array(bag2, 7);
    for (int i = 0; i < 7; i++) {
        state->bag[i] = bag1[i]; // Bag 1 through slots 0 to 6
    }
    for (int i = 0; i < 7; i++) {
        state->bag[i + 7] = bag2[i]; // Bag 2 through slots 7 to 13
    }
    state->bag_index = 0; // Set draw index

    // Start spawning first piece
    spawn_new_piece(state);
}
