#include "movement.h"
#include "board_control.h"

bool move_left(GameState* state)
{
    if (is_valid_pos(state, state->current.type, state->current.rot, state->current.x - 1, state->current.y)) {
        state->current.x--;
        state->last_action_rotation = false;
        return true;
    }
    return false;
}

bool move_right(GameState* state)
{
    if (is_valid_pos(state, state->current.type, state->current.rot, state->current.x + 1, state->current.y)) {
        state->current.x++;
        state->last_action_rotation = false;
        return true;
    }
    return false;
}

bool soft_drop(GameState* state)
{
    if (is_valid_pos(state, state->current.type, state->current.rot, state->current.x, state->current.y + 1)) {
        state->current.y++;
        state->last_action_rotation = false;
        return true;
    }
    return false;
}

int hard_drop(GameState* state)
{
    int rows = 0;
    while (is_valid_pos(state, state->current.type, state->current.rot, state->current.x, state->current.y + 1)) {
        state->current.y++;
        rows++;
    }
    return rows;
}
