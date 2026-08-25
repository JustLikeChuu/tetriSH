#include "engine.h"
#include "board_control.h"
#include "bag.h"
#include "garbage.h"

// Check for t-spin; return 1 if valid, 0 if not
int check_t_spin(GameState* state)
{
    // 1 & 2: Must be t-piece and last action must be rotation
    if (state->current.type != 3 || !state->last_action_rotation) {
        return 0;
    }

    // 3: 3 corner test
    int cx = state->current.x + 1;
    int cy = state->current.y + 1;
    int blocked_corners = 0;

    // Check for {x, y} offsets for the 4 corners (top left, top right, bottom left, bottom right)
    int corner_offsets[4][2] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
    // Through all 4 corners
    for (int i = 0; i < 4; i++) {
        // Checking x and y axis
        int check_x = cx + corner_offsets[i][0];
        int check_y = cy + corner_offsets[i][1];
        // If corner point is out of bounds
        if (check_x < 0 || check_x >= BOARD_WIDTH || check_y >= BOARD_HEIGHT || check_y < 0) {
            blocked_corners++;
        }
        // Within bounds but blocked
        else if (check_y >= 0 && state->board.cells[check_y][check_x] != 0) {
            blocked_corners++;
        }
    }
    // Minimum 3 corners blocked to qualify for t-spin
    if (blocked_corners >= 3) {
        return 1; // T-spin confirmed
    }

    return 0; // Not t-spin
}

// Function to advance the game / frame
int tick_game(GameState* state)
{
    // Attempt to let gravity pull the piece down
    if (is_valid_pos(state, state->current.type, state->current.rot, state->current.x, state->current.y + 1)) {
        state->current.y++; // If possible, fall for one row
        return 0;           // 0 lines cleared
    }

    // Game Over Check 1
    if (!is_valid_pos(state, state->current.type, state->current.rot, state->current.x, state->current.y + 1)) {
        // Lock Out
        if (state->current.y < 0) {
            state->game_over = true;
            return 0;
        }
    }

    // Check for t-spins
    bool tspin = check_t_spin(state);
    if (tspin) {
        state->t_spins++; // Increment counter
    }

    // Otherwise collision detected, so cannot move
    lock_piece(state);                // Bake into the board grid
    state->pieces_placed++;           // Increment coutner
    int cleared = clear_lines(state); // Clear full rows and store amount in counter
    if (cleared == 4) {
        state->tetrises++; // Increment counter
    }

    // Dealing with Garbage
    int damage = calculate_garbage(state, cleared, tspin); // Number of garbage lines generated in total for this turn
    // Cancel incoming garbage with our attack
    if (damage > 0 && state->pending_garbage > 0) // Generated + Incoming
    {
        if (damage >= (int)state->pending_garbage) // Generated > Incoming
        {
            damage -= (int)state->pending_garbage; // Reduce generated
            state->pending_garbage = 0;            // Reset Incoming
        } else {
            state->pending_garbage -= (uint32_t)damage; // Incoming > Generated
            damage = 0;                                 // Reset Generated
        }
    }
    // Send remaining generated damage to opponent
    state->outgoing_garbage = damage;
    // Taking damage
    if (cleared == 0 && state->pending_garbage > 0) // No generated to cancel out incoming
    {
        add_garbage(state, state->pending_garbage); // Push to bottom of the board
        state->pending_garbage = 0;                 // Reset buffer
    }

    // Spawn a new piece at the top AFTER garbage has been settled
    spawn_new_piece(state);

    // Game Over Check
    if (!is_valid_pos(state, state->current.type, state->current.rot, state->current.x, state->current.y)) {
        // Block Out
        state->game_over = true;
    }
    // Return lines cleared this turn
    return cleared;
}
// Advances gravity and lock delay by exactly one tick
bool update_timers(GameState* state)
{
    // Track current gravity of piece for lock delay
    int current_gravity = GRAVITY_THRESHOLD_START - ((state->level - 1) * 5);
    if (current_gravity < 5) {
        current_gravity = 5; // Failsafe
    }

    // Gravity + Lock Delay
    bool is_resting = !is_valid_pos(state, state->current.type, state->current.rot, state->current.x, state->current.y + 1);
    if (is_resting) {
        // Lock Timer
        state->lock_timer++;
        if (state->lock_timer >= LOCK_THRESHOLD_START) {
            tick_game(state);
            // Reset env variables
            state->lock_timer = 0;
            state->gravity_timer = 0;
            return true; // Piece locked, board definitely changed
        }
    } else {
        // Gravity Timer
        state->lock_timer = 0;
        state->gravity_timer++;
        if (state->gravity_timer >= current_gravity) {
            tick_game(state);
            state->gravity_timer = 0;
            return true; // Piece fell one row
        }
    }

    return false; // Nothing visible happened this tick
}
