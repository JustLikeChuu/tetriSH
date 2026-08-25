#ifndef TETRISBRAIN_ENGINE_H
#define TETRISBRAIN_ENGINE_H

#include "state.h"

/* ----- TIMING RULES ----- */
#define GRAVITY_THRESHOLD_START 50 // Loop cycles before the piece drops one row
#define LOCK_THRESHOLD_START 50    // Loop cycles a resting piece waits before locking

// Check for t-spin; return 1 if valid, 0 if not
int check_t_spin(GameState* state);

// Repeat function to advance the game
int tick_game(GameState* state);

// Advances gravity and lock delay by exactly one tick
bool update_timers(GameState* state);

#endif
