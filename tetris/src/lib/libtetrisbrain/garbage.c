#include "garbage.h"
#include <stdlib.h>

// Inject garbage lines at the bottom of the board
void add_garbage(GameState* state, int lines)
{
    // If 0 or -ve garbage passed, do nothing
    if (lines <= 0) {
        return;
    }

    // Bounds check in case garbage sent > board height
    if (lines > BOARD_HEIGHT) {
        lines = BOARD_HEIGHT;
    }

    // Bump all existing blocks up by the number of garbage lines
    for (int y = 0; y < BOARD_HEIGHT - lines; y++) // Loop from top of the board down to highest row
    {
        for (int x = 0; x < BOARD_WIDTH; x++) // Loop through x axis
        {
            state->board.cells[y][x] = state->board.cells[y + lines][x]; // Shift whole row up
        }
    }

    // Add the new garbage lines at the bottom
    for (int y = BOARD_HEIGHT - lines; y < BOARD_HEIGHT; y++) // Loop through newly created empty space at bottom of board
    {
        // Random x coord to spawn a hole
        int hole = rand() % BOARD_WIDTH;

        for (int x = 0; x < BOARD_WIDTH; x++) // Loop across x axis
        {
            if (x == hole) {
                // Gap
                state->board.cells[y][x] = 0;
            } else {
                // Fill with solid tile; used 8 as a separate ID since 0-7 already in use
                state->board.cells[y][x] = 8;
            }
        }
    }
}

// Queue incoming garbage so it accumulates in the pending meter until it is applied
void queue_garbage(GameState* state, int lines)
{
    if (lines <= 0) {
        return;
    }

    if (state == NULL) {
        return;
    }

    uint32_t pending = state->pending_garbage + (uint32_t)lines;
    if (pending > BOARD_HEIGHT) {
        pending = BOARD_HEIGHT;
    }

    state->pending_garbage = pending;
}

// Calculate garbage based on lines cleared (following TETR.IO guideline rules)
int calculate_garbage(GameState* state, int lines_cleared, bool is_t_spin)
{
    // Fake gamestate
    (void)state;
    // No lines cleared = 0 damage
    if (lines_cleared == 0) {
        state->combo = -1; // Reset combo counter
        return 0;
    }

    // Counters
    state->combo++; // Increment combo counter
    int base_garbage = 0;
    bool is_difficult_clear = is_t_spin || (lines_cleared == 4);

    // T spins deal more garbage (2x multiplier)
    if (is_t_spin) {
        if (lines_cleared > 0) {
            base_garbage = lines_cleared * 2;
        }
    } else {
        // Normal clears, regular multiplier
        if (lines_cleared == 2) {
            base_garbage = 1;
        } else if (lines_cleared == 3) {
            base_garbage = 3;
        } else if (lines_cleared == 4) {
            base_garbage = 4;
        }
    }

    // Specific clears have specific bonuses
    if (is_difficult_clear) {
        if (state->b2b) {
            base_garbage += 1;
        }
        state->b2b = true; // Maintain status
    } else {
        state->b2b = false; // Reset counter
    }

    // Guideline Combo Bonus to follow
    int combo_bonus = 0;
    if (state->combo > 0) {
        // Guideline Combo Table
        static const int COMBO_TABLE[] = {0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 5};
        if (state->combo < 12) {
            combo_bonus = COMBO_TABLE[state->combo];
        } else {
            combo_bonus = 5; // Capped for long combos
        }
    }

    return base_garbage + combo_bonus;
}
