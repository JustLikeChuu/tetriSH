#ifndef TETRISBRAIN_BOARD_H
#define TETRISBRAIN_BOARD_H

#include <stdint.h>

/* ----- BOARD ----- */
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20

typedef struct
{
    uint8_t cells[BOARD_HEIGHT][BOARD_WIDTH];
} Board;

#endif
