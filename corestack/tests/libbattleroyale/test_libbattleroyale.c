#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "libbattleroyale/server.h"
#include "libbattleroyale/client.h"

#define NUM_TEST_CLIENTS 2

enum TestStage {
    WAIT,
    SETUP,
    ESTABLISH_CONNECTION,
    SEND_MESSAGES,
    BROADCASTING,
    CLEANUP
}; // used by server to indicate which stage to start

enum TestResult {
    PASS,
    FAIL
}; // used by client to write success or failure

/*
This function is for the server to wait for all clients, returning 0 when all tests succeed and -1 if any of them fail
*/
int awaitTestResults(int statusPipe[2])
{
    int successes = 0;
    while (successes < NUM_TEST_CLIENTS) {
        enum TestResult result;
        read(statusPipe[0], &result, sizeof(enum TestResult));
        if (result == PASS) {
            printf("server: a client has passed the test\n");
            successes++;
            continue;
        }
        if (result == FAIL) {
            printf("server: a client has failed the test, returning\n");
            return -1;
        }
    }
    printf("server: all clients have passed the test\n");
    return 0;
}

/*
This function is for server to prompt clients to execute next stage of test
*/
int signalNextStage(int stagingPipe[NUM_TEST_CLIENTS][2], enum TestStage nextStage)
{
    for (int i = 0; i < NUM_TEST_CLIENTS; i++) {
        if (write(stagingPipe[i][1], &nextStage, sizeof(enum TestStage)) < 0) {
            printf("server: could not prompt the next test for a client\n");
            return -1;
        }
    }
    printf("server: prompted all clients\n");
    return 0;
}

/*
This function sends the result of a client test
*/
int signalTestResult(int signalPipe[2], enum TestResult result)
{
    if (write(signalPipe[1], &result, sizeof(enum TestResult)) < 0) {
        printf("client: could not pass server test results\n");
        return -1;
    }
    printf("client: passed server test result\n");
    return 0;
}

/*
This function waits for server prompt, an integer which can be cast into a TestStage enum
*/
int awaitInstruction(int stagingPipe[NUM_TEST_CLIENTS][2], int id)
{
    enum TestStage nextStage;
    if (read(stagingPipe[id][0], &nextStage, sizeof(enum TestStage)) < 0) {
        printf("client: could not receive next stage through pipe\n");
        return -1;
    }
    printf("client: received next stage instruction\n");
    return (int)nextStage;
}

/*
This functions waits for executes the specific test for client at a certain stage
*/
int executeTestStage(BRClient* testClient, int id, enum TestStage stage)
{
    enum TestResult result = FAIL;

    sleep(1);

    // execute test functions
    switch (stage) {
        // used in multiple tests
        unsigned char msgContent[2048];
    case SETUP:
        // do nothing
        result = PASS;
        break;
    case ESTABLISH_CONNECTION:
        // attempt a connection
        if (brclient_join(testClient, "127.0.0.1") < 0) {
            printf("client %d: failed to join the lobby\n", id);
            result = FAIL;
            break;
            break;
        }
        printf("client %d: successfully joined lobby\n", id);
        result = PASS;
        break;
    case SEND_MESSAGES:
        // build message
        snprintf(msgContent, 1024, "client %d: gamer word", id);
        // perform send
        if (brclient_send_msg(testClient, msgContent) < 0) {
            printf("client %d: could not send message\n", id);
            result = FAIL;
            break;
        }
        printf("client %d: successfully joined lobby\n", id);
        result = PASS;
        break;
    case BROADCASTING:
        // broadcasts are sent prior to this test running (should already be in queue)
        result = FAIL;
        while (brclient_get_app_msg(msgContent) != 0) {
            // break when reliable broadcast is matched
            if (strcmp(msgContent, "removed from game by anticheat (just a test phrase)") == 0) {
                printf("client %d: successfully received TCP broadcast\n", id);
                result = PASS;
                break;
            }
        }
        break;
    }

    return result;
}

/*
This test checks that all server and clients public functions which are used by developer can be used in the correct sequence
*/
int main(void)
{
    // setup pipes
    int stagingPipes[NUM_TEST_CLIENTS][2];
    int statusPipe[2];
    int id; // 0 for server, > 0 for clients
    enum TestStage currentStage = WAIT;

    if (pipe(statusPipe) < 0) {
        perror("test_libhtttp: pipe");
        printf("failed to create pipes\n");
        return -1;
    }

    pid_t pid = 1;

    // fork into 1 server and n clients
    for (int i = 0; i < NUM_TEST_CLIENTS; i++) {
        if (pipe(stagingPipes[i]) == -1) {
            perror("prompt pipe creation failed");
            exit(-1);
        }

        if (pid > 0) {
            pid = fork();
        }

        if (pid < 0) {
            perror("test_libhtttp: fork");
            printf("failed to client fork %d\n", i);
            exit(-1);
        }

        // close client process' unused pipe ends
        // should only read from own staging and write to status pipe
        if (pid == 0) {
            close(stagingPipes[i][1]);
            for (int j = 0; j < i; j++) {
                close(stagingPipes[j][0]);
            }
            close(statusPipe[0]);
            id = i;
            break;
        }
    }

    if (pid > 0) // parent - the server proc
    {
        // close write end of status pipe (used to listen for status from clients)
        close(statusPipe[1]);

        // setup the test server
        BRServer* testServer;
        if (brserver_init(&testServer) < 0) {
            printf("server: failed to create instance\n");
            goto server_cleanup;
        }
        printf("server: setup complete\n");

        // STAGE 1 - CONNECTION
        printf("server: starting test stage ESTABLISH_CONNECTION\n");
        if (brserver_open(testServer) < 0) {
            printf("server: failed to open lobby for client connections\n");
            goto server_cleanup;
        }
        printf("server: lobby is now open for client connections\n");
        if (signalNextStage(stagingPipes, ESTABLISH_CONNECTION) < 0) {
            printf("server: failed to prompt clients to start ESTABLISH_SERVER stage\n");
            goto server_cleanup;
        }
        if (awaitTestResults(statusPipe) < 0) {
            printf("server: failed client test in ESTABLISH_CONNECTION stage");
            goto server_cleanup;
        }

        printf("server: stage ESTABLISH_CONNETION complete, setting up next stage\n");

        // STAGE 2 - MESSAGE
        printf("server: starting test stage SEND_MESSAGES\n");
        if (brserver_start(testServer) < 0) {
            printf("server: failed to change server to game state");
            goto server_cleanup;
        }
        if (signalNextStage(stagingPipes, SEND_MESSAGES) < 0) {
            printf("server: failed to prompt clients to start SEND_MESSAGES stage\n");
            goto server_cleanup;
        }
        printf("server: GAME state, server now listening for client messages\n");
        if (awaitTestResults(statusPipe) < 0) {
            printf("server: a client failed in SEND_MESSAGES\n");
            goto server_cleanup;
        }

        // STAGE 3 - BROADCAST
        printf("server: starting test stage BROADCASTING\n");
        printf("server: GAME state, server now broadcasting messages to clients\n");
        if (brserver_send_broadcast(testServer, "banned for racism") < 0) {
            printf("server: failed to send UDP broadcast at BROADCASTING stage\n");
            goto server_cleanup;
        }
        if (brserver_send_to_all(testServer, "removed from game by anticheat (just a test phrase)") < 0) {
            printf("server: failed to send TCP broadcast at BROADCASTING stage\n");
            goto server_cleanup;
        }
        sleep(5);
        if (signalNextStage(stagingPipes, BROADCASTING) < 0) {
            printf("server: failed to prompt clients to start BROADCASTING stage\n");
            goto server_cleanup;
        }
        if (awaitTestResults(statusPipe) < 0) {
            printf("server: a client failed in BROADCASTING\n");
            goto server_cleanup;
        }
        printf("server: stage BROADCASTING complete, setting up next stage\n");

    // CLEANUP
    server_cleanup:
        if (signalNextStage(stagingPipes, CLEANUP) < 0) {
            printf("server: failed to prompt clients to start BROADCASTING stage\n");
            goto server_cleanup;
        }
        if (brserver_end(testServer) < 0) {
            printf("server: failed to enter END state\n");
        }
        wait(NULL); // for all children to exit
        free(testServer);
        return 0;
    }

    if (pid == 0) // child - a client proc
    {
        // setup the test client
        BRClient* testClient;

        if (brclient_init(&testClient) < 0) {
            printf("client %d: failed to create instance\n", id);
            goto client_cleanup;
        }
        printf("client %d: setup complete\n", id);

        // execute stage according to server
        do {
            currentStage = (enum TestStage)awaitInstruction(stagingPipes, id);
            if (currentStage < 0) {
                printf("client %d: failed to receive an instruction\n", id);
                goto client_cleanup;
            }
            enum TestResult result = (enum TestResult)executeTestStage(testClient, id, currentStage);
            if (result == FAIL) {
                printf("client %d: failed a test\n", id);
                goto client_cleanup;
            }
            if (signalTestResult(statusPipe, result) < 0) {
                printf("client %d: failed to signal server\n", id);
                goto client_cleanup;
            }
            printf("client %d: test passed, signalled server\n", id);
        } while (currentStage != CLEANUP);

    // CLEANUP
    client_cleanup:
        printf("client %d: test failed or CLEANUP reached, cleaning the client from memory\n", id);
        exit(0);
    }
}
