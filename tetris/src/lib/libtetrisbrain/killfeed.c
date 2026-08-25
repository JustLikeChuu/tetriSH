#include <stdio.h>
#include <inttypes.h>
#include "killfeed.h"

// Internal State
static KillFeedEntry feed[KILL_FEED_MAX]; // Fixed size array of entries
// Circular Buffer
static int feed_head = 0;  // Starting index where the new event will be written
static int feed_count = 0; // Counter to track how many active entries stored in the feed

// Public Functions
void add_kill_feed(uint32_t source, uint32_t target, uint32_t lines)
{
    // Write attacker ID, target ID and attack line count into array slot at the feed head
    feed[feed_head].source_player = source;
    feed[feed_head].target_player = target;
    feed[feed_head].lines = lines;

    // Modulo Arithmetic to move feed head to the next slot
    // Wraps around back to 0 if buffer is full to overwrite the oldest entry
    feed_head = (feed_head + 1) % KILL_FEED_MAX;
    // Increment counter until max
    if (feed_count < KILL_FEED_MAX) {
        feed_count++;
    }
}

// Print banner visuals
void draw_kill_feed(void)
{
    printf("                                           \n");
    printf("\n  <!> BATTLE LOG <!>\n");
    printf("  =========================================\n");
    // Iterate from oldest to newest
    for (int i = 0; i < feed_count; i++) {
        // Go to oldest entry, print to newest entry
        int index = (feed_head - feed_count + i + KILL_FEED_MAX) % KILL_FEED_MAX;
        KillFeedEntry* entry = &feed[index];
        printf("  \e[36mPlayer %u\e[0m sent \e[31m%u lines\e[0m to \e[33mPlayer %u\e[0m!\n", entry->source_player, entry->lines, entry->target_player);
    }

    // Padding the terminal
    for (int i = feed_count; i < KILL_FEED_MAX; i++) {
        printf("                                           \n");
    }
    printf("  =========================================\n");
}
