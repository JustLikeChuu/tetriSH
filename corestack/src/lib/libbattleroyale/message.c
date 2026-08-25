#include "message.h"

// private functions
// same functions as in common.c in PA2, not exposed to library users
int read_bytes(int sockfd, unsigned char** return_buf, uint64_t length)
{
    // allocate space for the return buffer
    if (*return_buf != NULL) {
        free(*return_buf);
        *return_buf = NULL;
    }
    *return_buf = malloc(length);
    if (*return_buf == NULL) {
        perror("[readBytes()] malloc");
        return -1;
    }

    // loop to receive length number of bytes and write to buffer
    uint64_t bytes_received = 0;
    while (bytes_received < length) {
        uint64_t remaining = length - bytes_received;
        ssize_t n = recv(sockfd, *return_buf + bytes_received, (size_t)remaining, 0);
        if (n <= 0) {
            perror("[readBytes()] recv");
            free(*return_buf);
            *return_buf = NULL;
            return -1;
        }
        bytes_received += (uint64_t)n;
    }

    return 0;
}

int send_bytes(int sockfd, const unsigned char* buf, uint64_t length)
{
    // loop to read length number of bytes from buf pointer location
    uint64_t total_sent = 0;
    while (total_sent < length) {
        uint64_t remaining = length - total_sent;
        ssize_t m = send(sockfd, buf + total_sent, (size_t)remaining, 0);
        if (m <= 0) {
            perror("[sendBytes()] send");
            return -1;
        }
        total_sent += (uint64_t)m;
    }
    return 0;
}

// public functions
// app layer message functions
int receive_message_tcp(int sockfd, Message* return_ptr)
{
    if (return_ptr == NULL) {
        LOG_E("[receiveMessageTCP()] pointer passed is not valid place in memory");
        goto fail;
    }

    LOG_I("[receiveMessageTCP()] preparing to receive message");

    // read Message header bytes
    LOG_D("[receiveMessageTCP()] listening for message...");
    unsigned char* buffer = NULL;
    if (read_bytes(sockfd, &buffer, sizeof(uint32_t)) < 0) {
        LOG_E("[receiveMessageTCP()] failed to read source id");
        goto fail;
    }
    uint32_t source_bytes;
    memcpy(&source_bytes, buffer, sizeof(source_bytes));
    return_ptr->source_id = ntohl(source_bytes);
    free(buffer);
    buffer = NULL;

    if (read_bytes(sockfd, &buffer, sizeof(uint32_t)) < 0) {
        LOG_E("[receiveMessageTCP()] failed to read message length");
        goto fail;
    }
    uint32_t length_bytes;
    memcpy(&length_bytes, buffer, sizeof(length_bytes));
    return_ptr->msg_len = ntohl(length_bytes);
    free(buffer);
    buffer = NULL;

    if (read_bytes(sockfd, &buffer, sizeof(uint32_t)) < 0) {
        LOG_E("[receiveMessageTCP()] failed to read message type");
        goto fail;
    }
    MessageType type_bytes;
    memcpy(&type_bytes, buffer, sizeof(type_bytes));
    return_ptr->msg_type = (MessageType)ntohl(type_bytes);
    free(buffer);
    buffer = NULL;

    if (read_bytes(sockfd, &buffer, MSG_CONTENT_LENGTH) < 0) {
        LOG_E("[receiveMessageTCP()] failed to read message content");
        goto fail;
    }
    // snprintf(returnPtr->msgContent, MSG_CONTENT_LENGTH, buffer);
    memcpy(return_ptr->msg_content, buffer, MSG_CONTENT_LENGTH); // Using memcpy to safely transfer binary bytes without null-byte termination issues
    free(buffer);
    buffer = NULL;

    LOG_D("[receiveMessageTCP()] received message:\n\tsource: %u\n\ttype: %d\ncontent: %s", return_ptr->source_id, return_ptr->msg_type, return_ptr->msg_content);
    LOG_I("[receiveMessageTCP()] finished receiving message");

    return 0;

fail:
    free(buffer);

    return -1;
}

int receive_message_udp(int sockfd, Message* return_ptr)
{
    LOG_I("[receiveMessageUDP()] preparing to receive broadcast...");

    // receiving message
    int flags_set = 0;
    // unsigned char *messageBytes = malloc(sizeof(Message)); // TODO fix the message size problem
    unsigned char message_bytes[sizeof(Message)]; // Stack buffer to prevent memory leaks
    // if (messageBytes == NULL)
    //{
    // perror("[receiveMessageUDP()] malloc");
    // return -1;
    //}

    LOG_D("[receiveMessageUDP()] waiting for bytes");

    if (recvfrom(sockfd, message_bytes, sizeof(Message), flags_set, NULL, NULL) < 0) // TODO specify message length to something reasonable
    {
        perror("[receiveAppMessageUDP()] recvfrom");
        return -1;
    }
    LOG_D("[receiveMessageUDP()] received raw message bytes");

    // fit bytes into message message struct
    // returnPtr = calloc(1, sizeof(Message));
    // Copy the bytes into the memory space provided by the caller
    memcpy(return_ptr, message_bytes, sizeof(Message));

    LOG_D("[receiveMessageUDP()] received broadcast with following message:\n\tsource: %u\n\ttype: %d", return_ptr->source_id, return_ptr->msg_type);
    LOG_I("[receiveMessageUDP()] broadcast has been received");

    return 0;
}

int send_message_tcp(int sockfd, const Message COMPLETE_MSG)
{
    LOG_I("[sendMessageTCP()] sending message:\n\tsourceId: %u\n\ttype (integerified): %d\n\tcontent: %s", COMPLETE_MSG.source_id, COMPLETE_MSG.msg_type, COMPLETE_MSG.msg_content);
    uint32_t source_id = htonl(COMPLETE_MSG.source_id);
    if (send_bytes(sockfd, (const unsigned char*)&source_id, sizeof(uint32_t)) < 0) {
        LOG_E("[sendMessageTCP()] failed to send source id");
        return -1;
    }
    uint32_t msg_len = htonl(COMPLETE_MSG.msg_len);
    if (send_bytes(sockfd, (const unsigned char*)&msg_len, sizeof(uint32_t)) < 0) {
        LOG_E("[sendMessageTCP()] failed to send message length");
        return -1;
    }
    MessageType type = htonl(COMPLETE_MSG.msg_type);
    if (send_bytes(sockfd, (const unsigned char*)&type, sizeof(uint32_t)) < 0) {
        LOG_E("[sendMessageTCP()] failed to send message type");
        return -1;
    }
    if (send_bytes(sockfd, COMPLETE_MSG.msg_content, MSG_CONTENT_LENGTH) < 0) {
        LOG_E("[sendMessageTCP()] failed to send message content");
        return -1;
    }
    LOG_I("[sendMessageTCP()] pushed all bytes though socket");
    return 0;
}

int send_broadcast_udp(int sockfd, const Message COMPLETE_MSG)
{
    LOG_I("[sendMessageUDP()] sending message:\n\tsourceId: %u\n\ttype (integerified): %d\n\tcontent: %s", COMPLETE_MSG.source_id, COMPLETE_MSG.msg_type, COMPLETE_MSG.msg_content);

    // preparing parameters for sendto()
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_UDP_BROAD),
        .sin_addr.s_addr = INADDR_BROADCAST // broadcast address
    };
    int flags_set = 0;
    socklen_t addr_len = sizeof(addr);

    // send the entire message over broadcast port
    if (sendto(sockfd, (unsigned char*)&COMPLETE_MSG, sizeof(Message), flags_set, (struct sockaddr*)&addr, addr_len) < 0) {
        perror("[sendMessageUDP()] sendto");
        return -1;
    }

    // cleanup

    LOG_I("[sendMessageUDP()] broadcast has been sent");
    return 0;
}

int send_unicast_udp(int sockfd, const char* dest_ip, const Message COMPLETE_MSG)
{
    LOG_I("[sendUnicastUDP()] sending message:\n\tsourceId: %u\n\ttype (integerified): %d\n\tcontent: %s", COMPLETE_MSG.source_id, COMPLETE_MSG.msg_type, COMPLETE_MSG.msg_content);

    // preparing parameters
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_UDP_UNI)};
    if (inet_pton(AF_INET, dest_ip, &addr.sin_addr) <= 0) {
        perror("[sendUnicastUDP()] inet_pton");
        return -1;
    }
    int flags_set = 0;
    socklen_t addr_len = sizeof(addr);

    // send to the unicast UDP port
    if (sendto(sockfd, (unsigned char*)&COMPLETE_MSG, sizeof(Message), flags_set, (struct sockaddr*)&addr, addr_len) < 0) {
        perror("[sendUnicastUDP()] sendto");
        return -1;
    }

    LOG_I("[sendUnicastUDP()] unicast message has been sent");
    return 0;
}
