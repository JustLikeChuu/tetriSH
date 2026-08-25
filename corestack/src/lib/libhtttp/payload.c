#include "libhtttp/payload.h"
#include <stdlib.h>

/* PAYLOADS */
void payload_encode_attack(char* buffer, const AttackPayload* payload)
{
    sprintf(buffer, "{source-player: %u, target-player: %u, lines: %u}", payload->source_player, payload->target_player, payload->lines);
}

void payload_decode_attack(const char* buffer, AttackPayload* payload)
{
    sscanf(buffer, "{source-player: %u, target-player: %u, lines: %u}", &payload->source_player, &payload->target_player, &payload->lines);
}

void payload_encode_roster(char* buffer, const RosterPayload* payload)
{
    int offset = sprintf(buffer, "{count: %u, ids: [", payload->count);
    for (uint32_t i = 0; i < payload->count; i++) {
        offset += sprintf(buffer + offset, "%u ", payload->ids[i]);
    }
    offset += sprintf(buffer + offset, "]}");
}

void payload_decode_roster(const char* buffer, RosterPayload* payload)
{
    sscanf(buffer, "{count: %u, ids: [", &payload->count);
    char* ptr = strstr(buffer, "ids: [") + strlen("ids: [");
    for (uint32_t i = 0; i < payload->count; i++) {
        sscanf(ptr, "%u ", &payload->ids[i]);
        ptr = strstr(ptr, " ") + 1;
    }
}

void payload_encode_input(char* buffer, const InputPayload* payload)
{
    sprintf(buffer, "{action-id: %u}", payload->action);
}

void payload_decode_input(const char* buffer, InputPayload* payload)
{
    sscanf(buffer, "{action-id: %u}", &payload->action);
}

void payload_encode_state(char* buffer, const StatePayload* payload)
{
    int offset = 0;
    for (int row = 0; row < 20; row++) {
        for (int col = 0; col < 10; col++) {
            buffer[offset++] = '0' + payload->board[row][col];
        }
    }

    sprintf(buffer + offset, ",%u,%u,%d,%d,%u,%u,%u,%u,%d,%u,%u,%u,%u,%u,%d,%u,%u,%u,%u,%u,%u,%u,%u",
            payload->current_type, payload->current_rot, payload->current_x, payload->current_y, payload->next_type,
            payload->preview[0], payload->preview[1], payload->preview[2],
            payload->held_type, payload->has_held,
            payload->player_id, payload->score, payload->level, payload->lines_cleared, payload->combo, payload->b2b, payload->t_spins, payload->tetrises, payload->pending_garbage,
            payload->target_player_id, payload->target_mode, payload->game_over, payload->winner_id);
}

void payload_decode_state(const char* buffer, StatePayload* payload)
{
    int offset = 0;
    for (int row = 0; row < 20; row++) {
        for (int col = 0; col < 10; col++) {
            payload->board[row][col] = (uint8_t)(buffer[offset++] - '0');
        }
    }

    sscanf(buffer + offset, ",%u,%u,%d,%d,%u,%u,%u,%u,%d,%u,%u,%u,%u,%u,%d,%u,%u,%u,%u,%u,%u,%u,%u",
           &payload->current_type, &payload->current_rot, &payload->current_x, &payload->current_y, &payload->next_type,
           &payload->preview[0], &payload->preview[1], &payload->preview[2],
           &payload->held_type, &payload->has_held,
           &payload->player_id, &payload->score, &payload->level, &payload->lines_cleared, &payload->combo, &payload->b2b, &payload->t_spins, &payload->tetrises, &payload->pending_garbage,
           &payload->target_player_id, &payload->target_mode, &payload->game_over, &payload->winner_id);
}
