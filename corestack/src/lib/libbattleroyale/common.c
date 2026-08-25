#include "common.h"

// checks file descriptors in Sockets struct
int check_sockets(Sockets socks)
{
    int tcp_active = socks.tcp > 0 ? 1 : 0;
    if (tcp_active == 0) {
        LOG_E("[checkSockets()] missing TCP socket");
        return 0;
    }
    int udp_uni_active = socks.udp_uni > 0 ? 1 : 0;
    if (udp_uni_active == 0) {
        LOG_E("[checkSockets()] missing UDP direct socket");
        return 0;
    }
    int udp_broad_active = socks.udp_broad > 0 ? 1 : 0;
    if (udp_broad_active == 0) {
        LOG_E("[checkSockets()] missing UDP broadcast socket");
        return 0;
    }
    return 1;
}

// closes both socket fd
int close_sockets(Sockets* socks)
{
    if (check_sockets(*socks) == 0) {
        LOG_E("[closeSockets()] sockets not created, no need to close");
        return -1;
    }
    close(socks->tcp);
    socks->tcp = -1;
    close(socks->udp_uni);
    socks->udp_uni = -1;
    close(socks->udp_broad);
    socks->udp_broad = -1;
    LOG_I("[closeSockets()] closed Sockets struct file descriptors successfully");
    return 0;
}

int free_sockets(Sockets** socks)
{
    Sockets* closing_sockets = *socks;
    if (close_sockets(closing_sockets)) {
        LOG_E("[closeSockets()] sockets could not be closed");
        return -1;
    }
    free(closing_sockets);
    // closingSockets = NULL;
    *socks = NULL; // Ensured caller's pointer is safely nullified
    LOG_I("[closeSockets()] freed Sockets struct in memory");
    return 0;
}

// sets up socket fd for both
int create_sockets(Sockets** socks)
{
    LOG_I("[createSockets()] creating sockets...");
    // Create tcp socket descriptor
    Sockets* new_socks = malloc(sizeof(Sockets));
    new_socks->tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (new_socks->tcp < 0) {
        perror("[createSockets()] socket");
        goto fail;
    }

    // create udp direct socket descriptor
    new_socks->udp_uni = socket(AF_INET, SOCK_DGRAM, 0);
    if (new_socks->udp_uni < 0) {
        perror("[createSockets()] socket");
        close(new_socks->tcp);
        goto fail;
    }

    // create udp broadcast socket descriptor
    new_socks->udp_broad = socket(AF_INET, SOCK_DGRAM, 0);
    if (new_socks->udp_broad < 0) {
        perror("[createSockets()] socket");
        close(new_socks->tcp);
        close(new_socks->udp_uni);
        goto fail;
    }

    LOG_I("[createSockets()] socket file descriptors created");
    *socks = new_socks;
    return 0;
fail:
    free(new_socks);
    new_socks = NULL;
    return -1;
}

// creates a new endpoint with sockets set up
int create_endpoint(Endpoint** endpt)
{
    LOG_I("[createEndpoint()] creating Endpoint...");
    Endpoint* new_endpt = malloc(sizeof(Endpoint));
    if (new_endpt == NULL) {
        perror("[common createEndpoint()] malloc");
        return -1;
    }

    // assign some default values
    new_endpt->id = 0;
    // memcpy(newEndpt->token, (unsigned char *)"0000000", NONCE_LEN);

    new_endpt->state = IDLE;

    // create new sockets
    if (create_sockets(&new_endpt->socks) < 0) {
        LOG_E("[createEndpoint()] could not create sockets for endpoint");
        goto fail;
    }

    // // create a message queue
    // newEndpt->messages = (MessageQueue){
    //     .front = NULL,
    //     .back = NULL
    // };

    *endpt = new_endpt;

    LOG_I("[createEndpoint()] endpoint created successfully"); // Moved above so it executes successfully

    return 0;

fail:
    free(new_endpt);
    return -1;
}

// smoothly close an endpoint
int free_endpoint(Endpoint** endpt)
{
    Endpoint* closing_endpt = *endpt;
    // close sockets
    if (free_sockets(&closing_endpt->socks) < 0) {
        LOG_E("[closeEndpoint()] could not close sockets");
        return -1;
    }
    closing_endpt->socks = NULL;

    // free struct memeory
    free(closing_endpt);
    // closingEndpt = NULL;
    *endpt = NULL;

    LOG_I("[closeEndpoint()] endpoint closed successfully");

    return 0;
}
