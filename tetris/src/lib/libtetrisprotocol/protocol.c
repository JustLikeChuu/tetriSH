#include <string.h>
#include "protocol.h"

/* ----- GAMESTATE <-> STATEPAYLOAD ----- */
// Server side, flattens the authoritative board into something sendable
void build_state_payload(const GameState* state, StatePayload* payload)
{
    memcpy(payload->board, state->board.cells, sizeof(payload->board));

    payload->current_type = (uint32_t)state->current.type;
    payload->current_rot = (uint32_t)state->current.rot;
    payload->current_x = (int32_t)state->current.x;
    payload->current_y = (int32_t)state->current.y;

    // Pull the same 3 pieces the client's preview box will draw
    for (int i = 0; i < 3; i++) {
        payload->preview[i] = (uint32_t)state->bag[(state->bag_index + i) % 14];
    }

    payload->held_type = (int32_t)state->held_type;
    payload->has_held = (uint32_t)state->has_held;

    payload->player_id = state->player_id;
    payload->score = (uint32_t)state->score;
    payload->level = (uint32_t)state->level;
    payload->lines_cleared = (uint32_t)state->lines_cleared;
    payload->combo = (int32_t)state->combo;
    payload->b2b = (uint32_t)state->b2b;
    payload->t_spins = (uint32_t)state->t_spins;
    payload->tetrises = (uint32_t)state->tetrises;
    payload->pending_garbage = state->pending_garbage;

    payload->target_player_id = state->target_player_id;
    payload->target_mode = (uint32_t)state->target_mode;
    payload->game_over = (uint32_t)state->game_over;
    payload->winner_id = state->winner_id;
}

// Client side, copies a received snapshot into the local GameState render mirror
// Only the fields the renderer reads are touched
void apply_state_payload(const StatePayload* payload, GameState* state)
{
    memcpy(state->board.cells, payload->board, sizeof(payload->board));

    state->current.type = (PieceType)payload->current_type;
    state->current.rot = (Rotation)payload->current_rot;
    state->current.x = (int)payload->current_x;
    state->current.y = (int)payload->current_y;

    // Rebuild just enough of the bag for the preview box to read normally
    // Index is pinned to 0 so bag[0], bag[1], bag[2] line up with the 3 sent pieces
    state->bag_index = 0;
    for (int i = 0; i < 3; i++) {
        state->bag[i] = (int)payload->preview[i];
    }

    state->held_type = (int)payload->held_type;
    state->has_held = (bool)payload->has_held;

    state->player_id = payload->player_id;
    state->score = (int)payload->score;
    state->level = (int)payload->level;
    state->lines_cleared = (int)payload->lines_cleared;
    state->combo = (int)payload->combo;
    state->b2b = (bool)payload->b2b;
    state->t_spins = (int)payload->t_spins;
    state->tetrises = (int)payload->tetrises;
    state->pending_garbage = payload->pending_garbage;

    state->target_player_id = payload->target_player_id;
    state->target_mode = (TargetingMode)payload->target_mode;
    state->game_over = (int)payload->game_over;
    state->winner_id = payload->winner_id;
}
