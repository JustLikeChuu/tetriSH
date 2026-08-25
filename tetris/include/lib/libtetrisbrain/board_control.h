#ifndef TETRISBRAIN_BOARD_CONTROL_H
#define TETRISBRAIN_BOARD_CONTROL_H

#include "state.h"
#include <stdbool.h>

/* ----- PIECE LOGISTICS ----- */
// Convert 2D (x,y) coordinates into 1D index for array
int get_rotation_index(int x, int y, Rotation rot);

// Wall kick helper function
bool test_rotate(GameState* state, int next_rot);

// Rotate clockwise logic
void rotate_current_piece(GameState* state);

// Rotate counter clockwise logic
void rotate_counter_clockwise(GameState* state);

/* ----- BOARD LOGISTICS ----- */
// Check for collisions
bool is_valid_pos(GameState* state, PieceType type, Rotation rot, int pos_x, int pos_y);

// Locking the piece after it finalizes its position
void lock_piece(GameState* state);

// Tetris
int clear_lines(GameState* state);

#endif
