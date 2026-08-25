#ifndef TETRISD_ROOM_H
#define TETRISD_ROOM_H

#include <stdint.h>
/**
 * @brief Runs one room's entire life inside a forked worker process
 * Binds its own TCP port, runs the lobby and the authoritative tick loop,
 * and reports to the master over hub_fd
 *
 * Never returns, exits the process when the room ends.
 * @param hub_fd current room's hub fd
 * @param tcp_port current port
 * @param id_base base number of room's player IDs
 * @param room_index Nth room
 */
void room_worker_run(int hub_fd, uint16_t tcp_port, uint32_t id_base, int room_index);

#endif
