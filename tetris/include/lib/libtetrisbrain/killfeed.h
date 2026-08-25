#ifndef TETRISBRAIN_KILLFEED_H
#define TETRISBRAIN_KILLFEED_H

#include <stdint.h>

// Max entries in the kill feed
#define KILL_FEED_MAX 4

// A single attack log entry
typedef struct
{
    uint32_t source_player; // Player that sent it
    uint32_t target_player; // Player that recieves it
    uint32_t lines;         // How much damage
} KillFeedEntry;

// Add an attack to the kill feed
void add_kill_feed(uint32_t source_player, uint32_t target_player, uint32_t lines);

// Render the kill feed
void draw_kill_feed(void);

#endif
