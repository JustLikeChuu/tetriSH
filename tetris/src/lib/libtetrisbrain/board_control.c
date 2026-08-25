#include "board_control.h"

#pragma region Piece Control
const int TETROMINOES[7][16] = {
    // Read-only 2D array => 7 pieces, 4x4 grid flattened into 1D list of numbers (16)
    // 1: solid piece, 0: empty space
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0}, // I
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0}, // O
    {0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0}, // T
    {0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0}, // S
    {0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0}, // Z
    {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0}, // J
    {0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0}  // L
};

// Convert 2D (x,y) coordinates into 1D index for array
int get_rotation_index(int x, int y, Rotation rot)
{
    // Determine rotation state
    switch (rot) {
    case ROT_0: // 0 degrees
        return x + (y * 4);
    case ROT_1: // 90 degrees CW
        return 12 + y - (x * 4);
    case ROT_2: // 180 degrees
        return 15 - (y * 4) - x;
    case ROT_3: // 270 degrees CCW
        return 3 - y + (x * 4);
    default:
        return 0;
    }
}

// Helper to identify rotation transition index (0 to 7)
static int get_transition_index(Rotation current_rot, Rotation next_rot)
{
    if (current_rot == ROT_0 && next_rot == ROT_1) {
        return 0; // 0 -> 90  (CW)
    }
    if (current_rot == ROT_1 && next_rot == ROT_0) {
        return 1; // 90 -> 0  (CCW)
    }
    if (current_rot == ROT_1 && next_rot == ROT_2) {
        return 2; // 90 -> 180 (CW)
    }
    if (current_rot == ROT_2 && next_rot == ROT_1) {
        return 3; // 180 -> 90 (CCW)
    }
    if (current_rot == ROT_2 && next_rot == ROT_3) {
        return 4; // 180 -> 270 (CW)
    }
    if (current_rot == ROT_3 && next_rot == ROT_2) {
        return 5; // 270 -> 180 (CCW)
    }
    if (current_rot == ROT_3 && next_rot == ROT_0) {
        return 6; // 270 -> 0   (CW)
    }
    if (current_rot == ROT_0 && next_rot == ROT_3) {
        return 7; // 0 -> 270   (CCW)
    }
    return 0;
}

// Wall kick helper function
bool test_rotate(GameState* state, int next_rot)
{
    // JLSTZ Kick Offsets (8 transitions x 5 tests x 2 coords {X, Y})
    static const int KICKS_JLSTZ[8][5][2] = {
        {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
        {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
        {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
        {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
        {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
        {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
        {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
        {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}};

    // I-Piece Kick Offsets
    static const int KICKS_I[8][5][2] = {
        {{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}},
        {{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}},
        {{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}},
        {{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}},
        {{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}},
        {{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}},
        {{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}},
        {{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}}};

    int trans_index = get_transition_index(state->current.rot, next_rot);
    PieceType type = state->current.type;

    for (int i = 0; i < 5; i++) {
        int offset_x, offset_y;
        // Select correct table
        if (type == 1) // I-Piece
        {
            offset_x = KICKS_I[trans_index][i][0];
            offset_y = KICKS_I[trans_index][i][1];
        } else if (type == 2) // O-Piece
        {
            offset_x = 0;
            offset_y = 0;
        } else // J, L, S, Z, T Pieces
        {
            offset_x = KICKS_JLSTZ[trans_index][i][0];
            offset_y = KICKS_JLSTZ[trans_index][i][1];
        }

        int nx = state->current.x + offset_x;
        int ny = state->current.y + offset_y;

        if (is_valid_pos(state, type, next_rot, nx, ny)) {
            state->current.x = nx;
            state->current.y = ny;
            state->current.rot = next_rot;
            state->last_action_rotation = true;
            return true;
        }
    }
    return false; // All 5 SRS kick tests failed
}

// Rotate clockwise logic
void rotate_current_piece(GameState* state)
{
    // Calculate what the next rotation state would be (0 -> 1 -> 2 -> 3 -> 0)
    Rotation next_rot = (Rotation)((state->current.rot + 1) % 4);
    test_rotate(state, next_rot);
}

// Rotate clockwise logic
void rotate_counter_clockwise(GameState* state)
{
    // Calculate what the next rotation state would be (3 -> 0 -> 1 -> 2 )
    int next_rot = (state->current.rot + 3) % 4;
    test_rotate(state, next_rot);
}
#pragma endregion

#pragma region Board Control
// Check for collisions
bool is_valid_pos(GameState* state, PieceType type, Rotation rot, int pos_x, int pos_y)
{
    int shape_index = type - 1; // Mapping directly to tetrominoes array

    // Keeping in bounds of 4x4
    for (int px = 0; px < 4; px++) {
        for (int py = 0; py < 4; py++) {
            int cell_index = get_rotation_index(px, py, rot); // Get exact 1D index for current rotation
            if (TETROMINOES[shape_index][cell_index] == 0)    // If this part of the 4x4 box is empty, do nothing
                continue;

            // Convert local 4x4 grid to global x / y positions
            int board_x = pos_x + px;
            int board_y = pos_y + py;

            // Out of bounds checks
            if (board_x < 0 || board_x >= BOARD_WIDTH || board_y >= BOARD_HEIGHT) {
                return false;
            }
            // Block Collision Check
            if (board_y >= 0 && state->board.cells[board_y][board_x] != 0) {
                return false;
            }
        }
    }
    // Move is valid
    return true;
}

// Locking the piece after it finalizes its position
void lock_piece(GameState* state)
{
    int shape_index = state->current.type - 1; // Mapping directly to tetrominoes array

    // Keeping in bounds of 4x4
    for (int px = 0; px < 4; px++) {
        for (int py = 0; py < 4; py++) {
            int cell_index = get_rotation_index(px, py, state->current.rot); // Which rotation?

            // Check if we are looking at a solid chunk of the piece / not empty space!!
            if (TETROMINOES[shape_index][cell_index] != 0) {
                // Find where the block lives on the board
                int board_x = state->current.x + px;
                int board_y = state->current.y + py;

                // Save final position onto the board (within bounds)
                if (board_y >= 0 && board_y < BOARD_HEIGHT) {
                    if (board_x >= 0 && board_x < BOARD_WIDTH) {
                        // Lock piece permanently onto the board grid
                        state->board.cells[board_y][board_x] = state->current.type;
                    }
                }
            }
        }
    }
}

// Clear lines
int clear_lines(GameState* state)
{
    // Collective counter for later use
    int lines_cleared = 0;

    // Once line is cleared, drop above row
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) // Check from bottom row up to row 0
    {
        // Checking if row is complete
        bool is_full = true;
        for (int x = 0; x < BOARD_WIDTH; x++) {
            // Empty space detected
            if (state->board.cells[y][x] == 0) {
                is_full = false;
                break;
            }
        }
        // No empty space detected
        if (is_full) {
            lines_cleared++; // Increment counter

            // Shift everything above this row down by 1
            for (int yy = y; yy > 0; yy--) // Loop through all rows from cleared row up to row 1
            {
                for (int xx = 0; xx < BOARD_WIDTH; xx++) // Loop across x axis
                {
                    // Shift content of one row above into current row
                    state->board.cells[yy][xx] = state->board.cells[yy - 1][xx];
                }
            }

            // Clear top row to 0
            for (int xx = 0; xx < BOARD_WIDTH; xx++) // Loop across x axis
            {
                state->board.cells[0][xx] = 0; // Clear entire row
            }

            // Check again if newly formed line is full
            y++; // Offset to check same row again
        }
    }

    // Update internal score/lines
    if (lines_cleared > 0) {
        state->lines_cleared += lines_cleared;
        state->level = (state->lines_cleared / 10) + 1; // Basic level progression
        state->score += lines_cleared * 100;            // Basic placeholder scoring
    }
    // Update cleared count
    return lines_cleared;
}
#pragma endregion
