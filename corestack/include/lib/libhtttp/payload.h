#ifndef LIBHTTTP_PAYLOAD_H
#define LIBHTTTP_PAYLOAD_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ----- PAYLOADS ----- */
// Attack (garbage) sent from one player to another
typedef struct {
    uint32_t source_player; // Player that sent it
    uint32_t target_player; // Player that recieves it
    uint32_t lines;         // How much damage
} AttackPayload;

// Broadcast once at game start so every client learns who else is in the lobby
typedef struct {
    uint32_t count; // Determined lobby size
    uint32_t ids[]; // Connected players
} RosterPayload;

// A single requested action from one client
typedef struct {
    uint32_t action;
} InputPayload;

// Authoritative snapshot of ONE player's board, pushed by the server every tick
typedef struct {
    uint8_t board[20][10]; // The locked-in playfield

    // Active pieceS (each field converted individually)
    uint32_t current_type;
    uint32_t current_rot;
    int32_t current_x;
    int32_t current_y;
    uint32_t next_type;

    // The 3 upcoming pieces the preview box renders
    uint32_t preview[3];

    // Hold box
    int32_t held_type;
    uint32_t has_held;

    // Dashboard counters
    uint32_t player_id;
    uint32_t score;
    uint32_t level;
    uint32_t lines_cleared;
    int32_t combo;
    uint32_t b2b;
    uint32_t t_spins;
    uint32_t tetrises;
    uint32_t pending_garbage;

    // Targeting display
    uint32_t target_player_id;
    uint32_t target_mode;

    uint32_t game_over;
    uint32_t winner_id;
} StatePayload;

/* ----- PACK / UNPACK ----- */
void payload_encode_attack(char* buffer, const AttackPayload* payload);
void payload_decode_attack(const char* buffer, AttackPayload* payload);

void payload_encode_roster(char* buffer, const RosterPayload* payload);
void payload_decode_roster(const char* buffer, RosterPayload* payload);

void payload_encode_input(char* buffer, const InputPayload* payload);
void payload_decode_input(const char* buffer, InputPayload* payload);

void payload_encode_state(char* buffer, const StatePayload* payload);
void payload_decode_state(const char* buffer, StatePayload* payload);

#endif
