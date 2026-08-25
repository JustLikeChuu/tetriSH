#include <sys/poll.h>

#include "lib/libbattleroyale/client.h"
#include "common.h"
#include "crypto.h"

static unsigned char sesskey[SESSION_KEY_LEN];
static char server_ip_addr[INET_ADDRSTRLEN];

static MessageQueue client_messages;
static pthread_mutex_t client_messages_lock;
static int pc_flags[2] = {0, 0};

// private functions
int connect_on_tcp(Sockets* socks, char* server_ip, uint16_t port)
{
    LOG_I("[connectToServer()] Attempting connection to server at %s:%u...", server_ip, port);
    // sockaddr_in of server to connect to
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) < 0) {
        perror("[client connectToServer()] inet_pton");
        return -1;
    }
    int server_fd = connect(socks->tcp, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (server_fd < 0) {
        perror("[client connectToServer()] connect");
        return -1;
    }
    LOG_I("[client connectToServer()] connection to server success");
    return server_fd;
}

int prepare_udp(Sockets* socks, char* server_ip)
{
    LOG_I("[prepareUnicastUDP()] preparing UDP unicast port");

    // set socket options
    int opt = 1;
    setsockopt(socks->udp_broad, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    // Bind sockets to port
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_UDP_BROAD),
        .sin_addr.s_addr = INADDR_ANY};
    if (bind(socks->udp_broad, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[prepareUnicastUDP()] bind");
        return -1;
    }
    LOG_D("[prepareUnicastUDP()] UDP socket bound to port %d", PORT_UDP_UNI);

    LOG_I("[prepareUnicastUDP()] UDP unicast port ready");

    return 0;
}

// connect()'s the unicast UDP socket to the server, so brclient_send_msg_udp
// can just send() without re-specifying the destination each call
int connect_udp_uni(Sockets* socks, char* server_ip)
{
    LOG_I("[connectUDPUni()] connecting unicast UDP socket to server at %s:%u...", server_ip, PORT_UDP_UNI);

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_UDP_UNI)};
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) < 0) {
        perror("[connectUDPUni()] inet_pton");
        return -1;
    }
    if (connect(socks->udp_uni, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[connectUDPUni()] connect");
        return -1;
    }

    LOG_I("[connectUDPUni()] unicast UDP socket connected");
    return 0;
}

// private state functions
typedef void (*StateLoops)(Endpoint* client);

/*
CLIENT in IDLE state where it is waiting for player to make action
In this state, do nothing
*/
// TODO possibly add server discovery
void client_idle_state(Endpoint* client)
{
    LOG_I("[idleStateLoop()] CLIENT entering IDLE state, awaiting state change...");
    while (client->state == IDLE) {
        // busy wait
        continue;
    }
    LOG_I("[idleStateLoop()] state change detected, CLIENT exiting IDLE state");
    return;
}

/*
CLIENT in LOBBY state where is awaits the server message to START
In this state, listen and make state change only upon START
*/
void client_lobby_state(Endpoint* client)
{
    LOG_I("[clientLobbyState()] CLIENT entering LOBBY state, awaiting state change...");
    while (client->state == LOBBY) {
        // Any HTTTP message in LOBBY state will be send through TCP
        Message msg;
        if (receive_message_tcp(client->socks->tcp, &msg) < 0) {
            LOG_E("[clientLobbyState()] CLIENT failed to receive TCP message");
            continue;
        }
        if (msg.msg_type == MSG_START) {
            LOG_D("[clientLobbyState()] CLIENT received message to START");
            client->state = GAME;
        }
    }
    LOG_I("[clientLobbyState()] state change detected, CLIENT exiting LOBBY state");
}

/*
CLIENT in GAME state where it listens for broadcasts, TCP, and UDP messages
In this state listen and handle messages accordingly (mainly about passing the messages to upper layer protocol)
*/
void client_game_state(Endpoint* client)
{
    LOG_I("[clientGameState()] CLIENT entering GAME state, awaiting state change...");

    // Any HTTTP message in LOBBY state will be sent through TCP, UDP unicast, or UDP broadcast
    // create pollfd struct
    struct pollfd* listen_fd = malloc(sizeof(struct pollfd) * 3);
    listen_fd[0] = (struct pollfd){
        .fd = client->socks->tcp,
        .events = POLLIN,
        .revents = 0};
    listen_fd[1] = (struct pollfd){
        .fd = client->socks->udp_uni,
        .events = POLLIN,
        .revents = 0};
    listen_fd[2] = (struct pollfd){
        .fd = client->socks->udp_broad,
        .events = POLLIN,
        .revents = 0};

    while (client->state == GAME) {
        int socket_activity = poll(listen_fd, 3, 50);
        if (socket_activity > 0) {
            LOG_D("[clientGameState()] %d active sockets on CLIENT", socket_activity);
            for (int i = 0; i < 3 && socket_activity > 0; i++) {
                if (listen_fd[i].revents & POLLIN) {
                    Message msg;

                    // receiving TCP message
                    if (i == 0) {
                        if (receive_message_tcp(listen_fd[i].fd, &msg) < 0) {
                            LOG_E("[clientGameState()] failed to receive TCP message from %d", listen_fd[i].fd);
                            listen_fd[i].fd = -1; // To prevent infinite CPU spin if server closes connection
                            continue;
                        }
                    }
                    // receiving UDP messages
                    else {
                        if (receive_message_udp(listen_fd[i].fd, &msg) < 0) {
                            LOG_E("[clientGameState()] failed to receive UDP message from %d", listen_fd[i].fd);
                            continue;
                        }
                    }

                    if (listen_fd[i].revents != 0) {
                        socket_activity--;
                    }

                    LOG_D("[clientGameState()] received message:\n\tsource: %u\n\ttype (integerified): %d\n\tcontent: %s", msg.source_id, msg.msg_type, msg.msg_content);
                    if (msg.msg_type == MSG_APP || msg.msg_type == MSG_APP_ENC) {
                        LOG_D("[clientGameState()] Message received for application");
                        if (msg.msg_type == MSG_APP_ENC) {
                            LOG_D("decrypting content");
                            unsigned char decrypted[MSG_CONTENT_LENGTH];
                            size_t decrypted_len = 0;
                            if (decrypt_message(&msg, client->sesskey, decrypted, &decrypted_len) < 0) {
                                LOG_E("[clientGameState()] failed to decrypt message from %d, dropping", listen_fd[i].fd);
                                continue;
                            }
                            msg.msg_len = (uint32_t)decrypted_len;
                            memcpy(msg.msg_content, decrypted, decrypted_len);
                            msg.msg_type = MSG_APP;
                        }
                        if (pc_flags[0]) {
                            pc_flags[1] = 1;
                            LOG_D("[clientGameState()] listener yielding CPU control for user polling queue");
                            sched_yield();
                        }
                        pthread_mutex_lock(&client_messages_lock);
                        Message_enqueue(&client_messages, msg);
                        pthread_mutex_unlock(&client_messages_lock);
                        pc_flags[1] = 0;
                    }
                    if (msg.msg_type == MSG_END) {
                        LOG_D("[clientGameState()] Message received to change to END state");
                        client->state = END;
                    }
                }
            }
        }
    }
    free(listen_fd); // Plug memory leak
    LOG_I("[clientGameState()] state change detected, CLIENT exiting GAME state");
}

void client_end_state(Endpoint* client)
{
    LOG_I("[endStateCleanup()] CLIENT entering END state, closing connection with all clients");
    if (free_endpoint(&client) < 0) {
        LOG_E("[clientEndState()] could not free client in memory");
    }
    LOG_I("[endStateCleanup()] CLIENT finished cleaning up");
}

// setup for private background thread function
StateLoops client_state_loops[] = {
    [IDLE] = client_idle_state,
    [LOBBY] = client_lobby_state,
    [GAME] = client_game_state,
    [END] = client_end_state};

void* client_thread_func(void* client)
{
    Endpoint* this_client = (Endpoint*)client;

    while (this_client->state != END) {
        client_state_loops[this_client->state](this_client);
    }

    if (this_client->state == END) {
        client_state_loops[this_client->state](this_client);
    }
    pthread_exit(NULL);
}

// public functions
// allows developers to create a libhtttp client in application
int brclient_init(BRClient** client_ptr)
{
    Endpoint* new_client = NULL;

    // create endpoint
    if (create_endpoint(&new_client) < 0) {
        LOG_E("[brclient_init()] could not create endpoint struct for client");
        return -1;
    }

    // open sockets
    if (create_sockets(&new_client->socks) < 0) {
        LOG_E("[brclient_init()] socket creation failed");
        return -1;
    }

    // default values
    new_client->id = 0;
    new_client->state = 0;

    *client_ptr = new_client;

    // prepare queue
    Message_init(&client_messages);
    pthread_mutex_init(&client_messages_lock, NULL);

    // spawn backrgound thread
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, client_thread_func, (void*)new_client) != 0) {
        perror("Failed to create thread");
        return 1;
    }

    LOG_I("[brclient_init()] new client created, and spawned background thread");

    return 0;
}

// connects to a libhtttp server
int brclient_join(BRClient* client_ptr, char* ip_address)
{
    Endpoint* this_client = client_ptr;

    LOG_I("[brclient_join()] attempting connection to lobby located at IP %s", ip_address);

    // save server IP for future unicast UDP messaging
    snprintf(server_ip_addr, sizeof(server_ip_addr), "%s", ip_address);

    // TALKING TO MASTER
    // the master only deals out rooms, connect and ask for one
    if (connect_on_tcp(this_client->socks, ip_address, PORT_TCP) < 0) {
        LOG_E("[startClientHandshake()] failed to connect to server at IP");
        return -1;
    }

    // master answers with the TCP port of the room worker this player was dealt into
    unsigned char* port_buffer = malloc(sizeof(uint32_t));
    if (port_buffer == NULL) {
        perror("[brclient_join()] malloc");
        return -1;
    }
    if (read_bytes(this_client->socks->tcp, &port_buffer, sizeof(uint32_t)) < 0) {
        LOG_E("[brclient_join()] failed to read room port from master");
        free(port_buffer);
        return -1;
    }
    uint32_t port_bytes;
    memcpy(&port_bytes, port_buffer, sizeof(port_bytes));
    uint16_t room_port = (uint16_t)ntohl(port_bytes);
    free(port_buffer);
    port_buffer = NULL;

    // TALKING TO ROOM WORKER
    // drop the master connection and reconnect straight to the assigned room
    LOG_I("[brclient_join()] master redirected us to room port %u, reconnecting...", room_port);
    close(this_client->socks->tcp);
    this_client->socks->tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (this_client->socks->tcp < 0) {
        perror("[brclient_join()] socket");
        return -1;
    }
    if (connect_on_tcp(this_client->socks, ip_address, room_port) < 0) {
        LOG_E("[brclient_join()] failed to connect to assigned room");
        return -1;
    }

    // receive user id and save to Endpoint
    unsigned char* buffer = NULL;
    buffer = malloc(sizeof(uint32_t));
    if (buffer == NULL) {
        perror("malloc");
        return -1;
    }
    if (read_bytes(this_client->socks->tcp, &buffer, sizeof(uint32_t)) < 0) {
        LOG_E("failed to read sourceId");
        return -1;
    }
    uint32_t source_bytes;
    memcpy(&source_bytes, buffer, sizeof(source_bytes));
    this_client->id = ntohl(source_bytes);
    free(buffer);
    buffer = NULL;

    // security - authentication
    // nonce generation
    unsigned char nonce[NONCE_LEN];
    RAND_bytes(nonce, sizeof(nonce));

    // receive server cert
    Message msg;
    do {
        receive_message_tcp(this_client->socks->tcp, &msg);
    } while (msg.msg_type != MSG_CERT);

    unsigned char* cert_bytes = malloc(msg.msg_len);
    X509* cert = load_cert_bytes(msg.msg_content, msg.msg_len);

    // send nonce
    msg.msg_type = MSG_AUTH;
    msg.source_id = this_client->id;
    snprintf(msg.msg_content, 2048, "%s", nonce);
    send_message_tcp(this_client->socks->tcp, msg);

    // receive signed nonce and verify
    do {
        receive_message_tcp(this_client->socks->tcp, &msg);
    } while (msg.msg_type != MSG_AUTH);

    if (!verify_message_pss(cert, msg.msg_content, msg.msg_len, nonce, sizeof(nonce))) {
        LOG_E("server could not be authenticated");
        return -1;
    }

    // authentication - symmetric key establishment
    // get public key
    EVP_PKEY* pubkey = X509_get_pubkey(cert);
    if (pubkey == NULL) {
        LOG_E("could not load pubkey from cert");
        return -1;
    }

    // generate session key and encrypt with pubkey
    if (generate_session_key(sesskey) < 0) {
        LOG_E("Failed key generation");
        return -1;
    }

    this_client->sesskey = malloc(SESSION_KEY_LEN);
    if (this_client->sesskey == NULL) {
        perror("malloc");
        return -1;
    }
    memcpy(this_client->sesskey, sesskey, SESSION_KEY_LEN);

    size_t enc_key_len;
    unsigned char* enc_key = rsa_encrypt_block(pubkey, sesskey, SESSION_KEY_LEN, &enc_key_len, 0);

    if (enc_key == NULL || enc_key_len > sizeof(msg.msg_content)) {
        LOG_E("failed to encrypt session key");
        free(enc_key);
        return -1;
    }

    // send key to server
    msg.source_id = this_client->id;
    msg.msg_type = MSG_KEY;
    msg.msg_len = enc_key_len;
    memcpy(msg.msg_content, enc_key, msg.msg_len);
    LOG_D("EEEE: %u", msg.msg_len);
    free(enc_key);
    if (send_message_tcp(this_client->socks->tcp, msg) < 0) {
        LOG_E("failed to send session key");
    }

    // INDEPENDENT OF SERVER
    // prepare UDP ports for future use upon connection
    if (prepare_udp(this_client->socks, ip_address) < 0) {
        LOG_E("[brclient_join()] could not prepare UDP port to receive broadcasts");
        return -1;
    }
    // Point the unicast UDP socket at the server so brclient_send_msg_udp can just send()
    if (connect_udp_uni(this_client->socks, ip_address) < 0) {
        LOG_E("[brclient_join()] could not connect unicast UDP socket to server");
        return -1;
    }

    // joining lobby successful, enter lobby state
    this_client->state = LOBBY;

    LOG_I("[brclient_join()] lobby joining complete");
    return 0;
}

// Client state getter
int brclient_get_state(BRClient* client_ptr)
{
    if (client_ptr == NULL) // Check for valid client
    {
        LOG_E("[brclient_get_state()] no valid client");
        return -1;
    }
    Endpoint* this_client = client_ptr;
    return this_client->state;
}

// message functions
int brclient_send_msg(BRClient* client_ptr, unsigned char content[MAX_APP_PAYLOAD_LEN])
{
    Endpoint* this_client = client_ptr;

    // build message
    Message msg = {
        .source_id = this_client->id,
        .msg_type = MSG_APP_ENC,
    };

    encrypt_message(&msg, this_client->sesskey, content, MAX_APP_PAYLOAD_LEN);

    // send via socket
    if (send_message_tcp(this_client->socks->tcp, msg) < 0) {
        LOG_E("[brclient_send_msg()] sending has failed");
        goto fail;
    }

    LOG_I("[brclient_send_msg()] message has been sent");

    return 0;

fail:
    return -1;
}

int brclient_send_msg_udp(BRClient* client_ptr, unsigned char content[2048])
{
    Endpoint* this_client = client_ptr;

    // build message
    Message msg = {
        .source_id = this_client->id,
        .msg_type = MSG_APP_ENC,
    };

    encrypt_message(&msg, this_client->sesskey, content, MAX_APP_PAYLOAD_LEN);

    // send message
    if (send_unicast_udp(this_client->socks->udp_uni, server_ip_addr, msg) < 0) {
        LOG_E("[brclient_send_msg_udp()] sending has failed");
        return -1;
    }

    LOG_I("[brclient_send_msg_udp()] message has been sent");

    return 0;
}

/*
function allows developers to get a message from the message queue
returns 0 if no message, returns 1 if there is
*/
int brclient_get_app_msg(unsigned char return_msg[2048])
{
    if (!Message_empty(&client_messages)) {
        if (pc_flags[1]) {
            pc_flags[0] = 1;
            LOG_D("[brclient_get_app_msg()] yielding CPU control for listener thread to write to queue");
            sched_yield();
        }
        pthread_mutex_lock(&client_messages_lock);
        memcpy(return_msg, Message_peek(&client_messages)->msg_content, MSG_CONTENT_LENGTH);
        Message_dequeue(&client_messages);
        pthread_mutex_unlock(&client_messages_lock);
        pc_flags[0] = 0;
        LOG_I("[brclient_get_app_msg()] client message has been returned to unsigned char array");
        return 1;
    } else {
        return 0;
    }
}

/*
Function returns the client id to a pointer
returns 0 on success, -1 on failure
*/
int brclient_get_id(BRClient* client_ptr, uint32_t* id)
{
    if (client_ptr == NULL) {
        LOG_E("[brclient_get_id()] no valid client");
        return -1;
    }
    Endpoint* this_client = client_ptr;
    *id = this_client->id;
    LOG_I("[brclient_get_id()] retrieved own client id %u", *id);
    return 0;
}
