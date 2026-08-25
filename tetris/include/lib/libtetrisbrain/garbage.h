#ifndef TETRISBRAIN_GARBAGE_H
#define TETRISBRAIN_GARBAGE_H

#include "state.h"

// Inject garbage lines at the bottom of the board
void add_garbage(GameState* state, int lines);

// Queue incoming garbage so it appears in the pending meter and is applied later
void queue_garbage(GameState* state, int lines);

// Calculate garbage based on lines cleared (following TETR.IO guideline rules)
int calculate_garbage(GameState* state, int lines_cleared, bool is_t_spin);

#endif
