#ifndef LIBBR_MESSAGE_H
#define LIBBR_MESSAGE_H

#include "common.h"

#define MSG_CONTENT_LENGTH 2048

typedef enum {
    // security related
    MSG_CERT,
    MSG_AUTH,
    MSG_KEY,
    // lobby related
    MSG_JOIN,
    MSG_LEAVE,
    MSG_START,
    MSG_END,
    // app related
    MSG_APP,    // plaintext
    MSG_APP_ENC // ciphertext
} MessageType;

typedef struct {
    uint32_t source_id;
    uint32_t msg_len;
    MessageType msg_type;
    unsigned char msg_content[MSG_CONTENT_LENGTH];
} Message;

DEFINE_QUEUE(Message, Message, 500);

// byte reading functions
int read_bytes(int sockfd, unsigned char** return_buf, uint64_t length);
int send_bytes(int sockfd, const unsigned char* buf, uint64_t length);

// messaging functions
int receive_message_tcp(int sockfd, Message* return_ptr);
int receive_message_udp(int sockfd, Message* return_ptr);
int send_message_tcp(int sockfd, const Message COMPLETE_MSG);
int send_broadcast_udp(int sockfd, const Message COMPLETE_MSG);
int send_unicast_udp(int sockfd, const char* dest_ip, const Message COMPLETE_MSG);

#endif
