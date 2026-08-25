#ifndef TETRISU_EVENTS_H
#define TETRISU_EVENTS_H

#include <stdint.h>
#include "lib/libtetrisprotocol/protocol.h"
#include "lib/libhtttp/payload.h"

// Event Dictionary
enum GameEvents {
    EVENT_ATTACK_GENERATED,
    EVENT_COUNT
};

// Network Routing
void on_attack_generated(void* args);

#endif
