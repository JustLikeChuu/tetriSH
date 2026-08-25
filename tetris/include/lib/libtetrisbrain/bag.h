#ifndef TETRISBRAIN_BAG_H
#define TETRISBRAIN_BAG_H

#include "state.h"

// Helper method to shuffle arrays
void shuffle_array(int* array, int size);

// Shifts the upcoming bag forward and generates a new one
void refill_bag(GameState* state);

// Spawns a new piece
void spawn_new_piece(GameState* state);

#endif
