#ifndef LIBBR_CLIENT_H
#define LIBBR_CLIENT_H

#include <stdint.h>

#define MSG_CONTENT_LENGTH 2048

typedef void BRClient;

// States to identify client state
typedef enum {
    BRCLIENT_STATE_IDLE = 0,  // Connected but not doing anything yet
    BRCLIENT_STATE_LOBBY = 1, // Waiting in lobby for game to start
    BRCLIENT_STATE_GAME = 2,  // Actively playing a match
    BRCLIENT_STATE_END = 3    // Match ended
} BRClientState;

int brclient_init(BRClient** client_ptr);
int brclient_join(BRClient* client_ptr, char* ip_address);
int brclient_get_state(BRClient* client_ptr);
// int leaveLobby(LibhtttpClient *clientPtr);

int brclient_send_msg(BRClient* client_ptr, unsigned char content[MSG_CONTENT_LENGTH]); // use defined value instead of explicit number
int brclient_send_msg_udp(BRClient* client_ptr, unsigned char content[MSG_CONTENT_LENGTH]);

int brclient_get_app_msg(unsigned char return_msg[MSG_CONTENT_LENGTH]);

int brclient_get_id(BRClient* client_ptr, uint32_t* id); // returns 0 on success, -1 on failure; writes id via out-param

#endif
