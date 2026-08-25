#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "lib/libeventbus.h"
#include "config.h"
#include "events.h"
#include "input.h"
#include "renderer.h"
#include "input_handler.h"
#include "lib/libtetrisprotocol/protocol.h"
#include "lib/libhtttp.h"
#include "lib/libtetrisbrain/state.h"
#include "lib/libtetrisbrain/targeting.h"
#include "lib/libtetrisbrain/killfeed.h"
#include "lib/libbattleroyale/client.h"

// How long to wait for the host to start the match before giving up (1 minutes)
#define LOBBY_WAIT_TIMEOUT_MS (300 * 1000)

// Global network client
BRClient* network_client = NULL;

// Populate lobby from server's REQ_ROSTER broadcast at game start
Roster lobby = {0};

// Helper function to trim \n
static void trim_newline(char* s)
{
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
    }
}

// --- MAIN GAME LOOP ---
int main(void)
{
    // PATCH FIX FOR FLICKERING TERMINAL
    setvbuf(stdout, NULL, _IOFBF, 16384);

    // Clear terminal screen
    printf("\e[1;1H\e[2J");
    fflush(stdout);

    // Network Client Initialization
    char server_ip[INET_ADDRSTRLEN + 2] = {0}; // To store server IPV4 addr
    while (true) {
        printf("Enter server IP (default 127.0.0.1): ");
        fflush(stdout); // Prompt must abbear before we block on input

        // Get user input for server IPV4 address
        if (fgets(server_ip, sizeof(server_ip), stdin) == NULL) // Failed
        {
            printf("[tetrisu] Input error.\n");
            return -1;
        }

        // Oversized input leaves the rest of the line in stdin, throw it away so it cannot leak into the next prompt
        if (strchr(server_ip, '\n') == NULL) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {
                // Discard
            }
        }

        trim_newline(server_ip);  // Remove \n from input
        if (server_ip[0] == '\0') // No input detected
        {
            strcpy(server_ip, "127.0.0.1"); // Set as local host
            break;
        }

        struct in_addr addr;
        if (inet_pton(AF_INET, server_ip, &addr) == 1) // Verify input if is actual IPV4 addr
        {
            break;
        }
        printf("[tetrisu] Invalid IP address '%s'. Please enter a valid IPv4 address.\n", server_ip); // Otherwise invalid input, prompt to retry
    }

    if (brclient_init(&network_client) < 0) {
        printf("[tetrisu] Failed to create network client.\n");
        return -1;
    } else if (brclient_join(network_client, server_ip) < 0) {
        printf("[tetrisu] Failed to join lobby at %s.\n", server_ip);
        printf("[tetrisu] Check the server is running and the IP is correct.\n");
        return -1;
    } else {
        printf("[tetrisu] Connected to lobby at %s successfully!\n", server_ip);
    }

    // Wait for host to start the match
    // Allow time for server to reach LOBBY_SIZE and enter GAME state
    printf("[tetrisu] Waiting for lobby to fill and game to start...\n");
    int waited_ms = 0;
    while (brclient_get_state(network_client) != BRCLIENT_STATE_GAME) // Player is not in a game yet
    {
        usleep(100000); // Wait for 100ms before polling again
        waited_ms += 100;

        // To prevent client from hanging forever
        if (waited_ms >= LOBBY_WAIT_TIMEOUT_MS) {
            printf("[tetrisu] Timed out after %d seconds waiting for the game to start.\n", LOBBY_WAIT_TIMEOUT_MS / 1000);
            return -1;
        }
    }
    printf("[tetrisu] Game started!\n"); // Game start flag

    // Clear terminal screen
    // Set up the terminal for the game
    enable_raw_mode();
    printf("\e[1;1H\e[2J");
    fflush(stdout);

    start_game(&gamestate_player);
    if (brclient_get_id(network_client, &gamestate_player.player_id) < 0) // Server-assigned ID retrieval
    {
        printf("[tetrisu] Failed to retrieve player ID from server.\n");
        gamestate_player.game_over = true; // Bail out cleanly rather than run with an uninitialized ID
    }

    // Event Bus setup -> drives the local kill feed
    event_bus_init(EVENT_COUNT);
    event_bus_listen(EVENT_ATTACK_GENERATED, on_attack_generated);

    // --- THE GAME LOOP ---
    while (!gamestate_player.game_over) {
        // Deal with active inputs
        process_inputs(&gamestate_player);

        // Read directly from message queue
        HyperText net_buffer = {0};

        while (brclient_get_app_msg((unsigned char*)net_buffer) == 1) {
            ParsedMsgHT parsed = {0};
            if (parse_hypertext(net_buffer, &parsed) < 0) {
                continue; // Malformed message, drop it
            }

            MethodHTTTP method;
            uint32_t sender_id;
            char* body;
            req_extract_info(&parsed, &method, &sender_id, &body);

            if (method == REQ_STATE) // Authoritative board pushed by the server
            {
                StatePayload incoming;
                payload_decode_state(body, &incoming);

                if (incoming.player_id == gamestate_player.player_id) {
                    apply_state_payload(&incoming, &gamestate_player);
                }
            } else if (method == REQ_ROSTER) // Server to populate who is in the lobby
            {
                struct {
                    uint32_t count;
                    uint32_t ids[MAX_LOBBY_SIZE];
                } incoming_roster = {0};
                payload_decode_roster(body, (RosterPayload*)&incoming_roster);

                lobby.count = incoming_roster.count;
                if (lobby.count > MAX_LOBBY_PLAYERS) {
                    lobby.count = MAX_LOBBY_PLAYERS; // Failsafe (technically shouldn't happen)
                }
                for (int i = 0; i < lobby.count; i++) {
                    lobby.ids[i] = incoming_roster.ids[i];
                    lobby.eliminated[i] = false; // Nobody is out yet at game start
                }
            } else if (method == REQ_ATTACK) // Notification only, damage already applied server-side
            {
                AttackPayload incoming;
                payload_decode_attack(body, &incoming);

                // Kill feed is driven through the event bus
                event_bus_trigger(EVENT_ATTACK_GENERATED, &incoming);
            }
        }

        // Render whatever the server last told us
        draw_board(&gamestate_player);

        // Delay frames to be visible to the human eye
        usleep(DELAY_MICROSECONDS);
    }

    // Clean up after game ends
    draw_board(&gamestate_player);
    printf("\n\n");
    printf("<!> ====================== <!>\n");
    printf("<!>       GAME OVER!       <!>\n");
    printf("<!> ====================== <!>\n");

    // Showcase Winner Segment
    if (gamestate_player.winner_id != 0) // Winner has been decided
    {
        if (gamestate_player.winner_id == gamestate_player.player_id) // For player that is the winner
        {
            printf("\n*** CONGRATULATIONS! You won as P%u! ***\n", gamestate_player.winner_id);
        } else {
            // Everyone else
            printf("\n*** Winner: P%u ***\n", gamestate_player.winner_id);
        }
    } else {
        // Draw
        printf("\n*** No survivors. Match ended with no winner. ***\n");
    }
    printf("\nPress any key to exit...\n");
    fflush(stdout);

    while (!kbhit()) {
        usleep(DELAY_MICROSECONDS);
    }
    getchar();

    return 0;
}
