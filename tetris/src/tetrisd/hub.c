#include <errno.h>
#include <sys/socket.h>
#include "hub.h"

int hub_send(int fd, const HubMsg* msg)
{
    // Write bytes to the socket, returns how many actually went out (or -1)
    // NOSIGNAL so a dead peer surfaces as an error instead of killing the process
    ssize_t sent = send(fd, msg, sizeof(*msg), MSG_NOSIGNAL);

    // Did it send the whole struct (by compare memory size)?
    // If not, it failed! As SEQPACKET is all-or-nothing
    return sent == (ssize_t)sizeof(*msg) ? 0 : -1;
}

int hub_recv(int fd, HubMsg* msg)
{
    // Read bytes off the socket into msg, returns how many (or -1)
    // DONTWAIT makes it return immediately instead of blocking when nothing's waiting
    ssize_t got = recv(fd, msg, sizeof(*msg), MSG_DONTWAIT);

    // (1) Exactly 1 message
    if (got == (ssize_t)sizeof(*msg)) {
        return 1;
    }

    // (2) Nothing to read right now, case DONTWAIT Produces, not real error
    if (got < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return 0;
    }
    return -1; // (3) Peer closed or hard error
}
