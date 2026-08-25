#include "hold.h"
#include "bag.h"

// Hold piece
void hold_piece(GameState* state)
{
    // If a piece is already held, do nothing
    if (state->has_held) {
        return;
    }

    // If not holding anything, store the piece and spawn a new one
    if (state->held_type == 0) {
        state->held_type = state->current.type; // Copy shape ID into hold slot
        spawn_new_piece(state);
    } else // Swap pieces
    {
        int temp = state->current.type;
        state->current.type = state->held_type;
        state->held_type = temp;

        // Reset to spawn at top of board
        state->current.y = -2;
        state->current.x = BOARD_WIDTH / 2 - 2;
        state->current.rot = 0;
    }

    // Set flag to true to prevent holding new pieces for this turn
    state->has_held = true;
}
