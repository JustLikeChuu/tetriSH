#ifndef TETRISPROTOCOL_PROTOCOL_H
#define TETRISPROTOCOL_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "lib/libtetrisbrain/state.h"
#include "lib/libhtttp/payload.h"

/* ----- SHARED LIMITS ----- */
#define MAX_LOBBY_SIZE 2 // TO BE KEPT IN SYNC with libtetrisbrain's MAX_LOBBY_PLAYERS

/* ----- PLAYER ACTIONS ----- */
// Every key press the client can ask the server to perform on its behalf
typedef enum {
    ACTION_NONE = 0,
    ACTION_MOVE_LEFT,
    ACTION_MOVE_RIGHT,
    ACTION_ROTATE_CW,
    ACTION_ROTATE_CCW,
    ACTION_SOFT_DROP,
    ACTION_HARD_DROP,
    ACTION_HOLD,
    ACTION_CYCLE_TARGET_MODE, // 'T' key
    ACTION_CYCLE_TARGET,      // 'R' key
    ACTION_QUIT               // 'Q' key
} PlayerAction;

/* ----- GAMESTATE <-> STATEPAYLOAD ----- */
// AttackPayload / RosterPayload / InputPayload / StatePayload now live in
// lib/libhtttp/payload.h, sent over HTTTP instead of these raw tagged packets
// Server side: flatten its authoritative GameState into a sendable snapshot
void build_state_payload(const GameState* state, StatePayload* payload);

// Client side: copy a received snapshot into the local GameState render mirror
void apply_state_payload(const StatePayload* payload, GameState* state);

#endif
