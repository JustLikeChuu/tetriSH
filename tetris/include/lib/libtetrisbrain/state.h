#ifndef TETRISBRAIN_STATE_H
#define TETRISBRAIN_STATE_H

#include "board.h"
#include "piece.h"
#include <stdbool.h>
#include <stdint.h>

/* ----- GAME STATE ----- */
typedef enum {
    TARGET_MANUAL, // Manual cycling
    TARGET_RANDOM, // Random opponent
    TARGET_KO,     // Highest stack
    TARGET_MODE_COUNT
} TargetingMode;

typedef struct
{
    Board board;
    Piece current;
    Piece next;
    TargetingMode target_mode;
    uint32_t player_id;
    uint32_t target_player_id;
    int gravity_timer;
    int lock_timer;
    int held_type;
    bool has_held;
    int bag[14];
    int bag_index;
    int pieces_placed;
    int tetrises;
    int t_spins;
    bool last_action_rotation;
    uint32_t pending_garbage;
    uint32_t outgoing_garbage;
    uint32_t last_attacker_id;
    int score;
    int lines_cleared;
    int combo;
    bool b2b;
    int level;
    int game_over;
    uint32_t winner_id;
} GameState;

// Establish game state
extern GameState gamestate_player;

// Function call to start the game
void start_game(GameState* state);

#endif
