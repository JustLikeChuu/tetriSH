#include <stddef.h>
#include <string.h>
#include "events.h"
#include "lib/libtetrisbrain/state.h"
#include "lib/libtetrisbrain/killfeed.h"
#include "lib/libbattleroyale/client.h"

// Instantiate network client
extern BRClient* network_client;

// Kill feed handler
// Fires when an attack notification arrives from the server so the local kill feed can display it
void on_attack_generated(void* args)
{
    // Arm the payload
    AttackPayload* attack = (AttackPayload*)args;

    // Update kill feed (locally)
    add_kill_feed(attack->source_player, attack->target_player, attack->lines);
}
