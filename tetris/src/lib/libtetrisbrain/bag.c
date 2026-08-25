#include "bag.h"
#include <stdlib.h>

// Fisher-Yates Algorithm Helper
void shuffle_array(int* array, int size)
{
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

// Shifts the upcoming bag forward and generates a new one
void refill_bag(GameState* state)
{
    for (int i = 0; i < 7; i++) {
        state->bag[i] = state->bag[i + 7]; // Copy last 7 pieces and replace the first 7 pieces
    }

    int new_bag[7] = {1, 2, 3, 4, 5, 6, 7}; // Generate new bag with the 7 pieces
    shuffle_array(new_bag, 7);              // Uniform shuffle using FYA
    for (int i = 0; i < 7; i++) {
        state->bag[i + 7] = new_bag[i]; // Add the new 7 pieces into the bag
    }

    state->bag_index = 0; // Reset pointer to track from start again
}

// Spawns a new piece
void spawn_new_piece(GameState* state)
{
    // Draw next piece from the bag then advance the pointer
    // state->bag_index to specify index
    // state->bag to select specified piece at specified index
    // state->current.type to assign piece based on index and type
    state->current.type = state->bag[state->bag_index];
    state->bag_index++;

    // Top up the bag with new pieces once 7th piece is taken from the bag
    if (state->bag_index >= 7) {
        refill_bag(state);
    }

    // Set spawn variables
    state->current.rot = ROT_0;               // Default rotation
    state->current.x = (BOARD_WIDTH / 2) - 2; // Centered horizontally
    state->current.y = -2;                    // Top of the board (nudged it above to allow for top out collision checks)
    state->has_held = false;                  // Ensure player is not holding any piece at start
}
