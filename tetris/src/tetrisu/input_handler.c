#include <stdio.h>
#include <string.h>
#include "input.h"
#include "events.h"
#include "config.h"
#include "input_handler.h"
#include "lib/libtetrisprotocol/protocol.h"
#include "lib/libtetrisbrain/state.h"
#include "lib/libbattleroyale/client.h"
#include "lib/libhtttp.h"

// Instantiate network client
extern BRClient* network_client;

// Ships one requested action to the server
static void send_action(GameState* state, PlayerAction action)
{
    if (network_client == NULL) {
        return; // No connection, nothing to ask
    }

    InputPayload payload = {.action = (uint32_t)action};

    ParsedMsgHT msg = {0};
    req_create_action(state->player_id, REQ_ACTION, &payload, &msg);

    HyperText ht;
    convert_to_hypertext(&msg, ht);

    brclient_send_msg(network_client, (unsigned char*)ht);
}

// Process all pending terminal inputs
// Every branch is now a pure keybind -> action mapping
void process_inputs(GameState* state)
{
    // Read user inputs
    while (kbhit()) {
        // Call getchar() wrapper
        int key = getchar();

        // Player keybinds
        // Linux arrow keys send 3 characters instantly => escape (27), '[', and a letter
        if (key == 27) // Escape or arrow key?
        {
            if (kbhit() && getchar() == '[') // Bracket right after?
            {
                if (kbhit()) // Decide output based on letter
                {
                    switch (getchar()) {
                    case 'A': // Up arrow (rotate clockwise)
                        send_action(state, ACTION_ROTATE_CW);
                        break;
                    case 'D': // Left arrow
                        send_action(state, ACTION_MOVE_LEFT);
                        break;
                    case 'C': // Right arrow
                        send_action(state, ACTION_MOVE_RIGHT);
                        break;
                    case 'B': // Down arrow (soft drop + lock delay)
                        send_action(state, ACTION_SOFT_DROP);
                        break;
                    }
                }
            }
        } else if (key == 'x' || key == 'X') // Rotate clockwise (alternate key)
        {
            send_action(state, ACTION_ROTATE_CW);
        } else if (key == 'z' || key == 'Z') // Rotate counterclockwise
        {
            send_action(state, ACTION_ROTATE_CCW);
        } else if (key == 't' || key == 'T') // Change target mode
        {
            send_action(state, ACTION_CYCLE_TARGET_MODE);
        } else if (key == 'r' || key == 'R') // Change target ID
        {
            send_action(state, ACTION_CYCLE_TARGET);
        } else if (key == ' ') // Spacebar (hard drop)
        {
            send_action(state, ACTION_HARD_DROP);
        } else if (key == 'h' || key == 'H') // H to hold
        {
            send_action(state, ACTION_HOLD);
        } else if (key == 'q' || key == 'Q') // Q to quit
        {
            // Tell the server we are leaving so it frees our slot and stops anyone from targeting us, then close down locally
            send_action(state, ACTION_QUIT);
            state->game_over = true;
            break; // Exit
        }
    }
}
