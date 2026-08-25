#ifndef TETRISBRAIN_MOVEMENT_H
#define TETRISBRAIN_MOVEMENT_H

#include <stdbool.h>
#include "state.h"

/* INPUTS */
bool move_left(GameState* state);

bool move_right(GameState* state);

bool soft_drop(GameState* state);

int hard_drop(GameState* state);

#endif
