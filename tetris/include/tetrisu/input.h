#ifndef TETRISU_INPUT_H
#define TETRISU_INPUT_H

// Invoke terminal settings
void enable_raw_mode();

// Reset terminal back to normal; if not will remain broken
void disable_raw_mode(void);

// Checks if a key has been pressed (Non-blocking)
int kbhit(void);

#endif
