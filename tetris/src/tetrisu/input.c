#include <stdio.h>
#include <stdlib.h>
#include <termios.h> // Linux terminal control
#include <unistd.h>  // POSIX OS API
#include <fcntl.h>   // File control (for non-blocking reads)
#include "input.h"

// --- TERMINAL CONTROL (termios) ---
// Linux in-built terminal flags
struct termios orig_termios; // Store terminal initial settings before the game starts

// Reset terminal back to normal; if not will remain broken
void disable_raw_mode(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios); // Apply stdin NOW, then reset back to handed saved state
    printf("\e[?25h");                               // Show cursor
    fflush(stdout);                                  // Force print
}

// Invoke terminal settings
void enable_raw_mode()
{
    tcgetattr(STDIN_FILENO, &orig_termios); // Save current terminal settings and store as saved state
    atexit(disable_raw_mode);               // Enforces revert back to normal terminal settings when user quits / ctrl-c
    struct termios raw = orig_termios;      // Make a copy of current state for us to work on

    // ICANON => system holds input inside a buffer until enter key is pressed
    // When disabled, sends every single keypress directly to the application instantly
    // ECHO => shows current input on screen; when disabled, input is completely hidden from screen
    raw.c_lflag &= ~(ECHO | ICANON); // Turn off "print keys to screen" and "wait for enter key"

    tcsetattr(STDIN_FILENO, TCSANOW, &raw); // Apply modifications on this saved state copy
    printf("\e[?25l");                      // Hide blinking cursor
    fflush(stdout);                         // Force print
}

// Checks if a key has been pressed (Non-blocking)
int kbhit(void)
{
    int ch;

    // In Linux, input is also treated as a file;
    int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);      // Get current fd for stdin
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK); // Add special flag => if no input registered, instantly return EOF and continue
    ch = getchar();                                  // Attempt to read a chara byte; if key was pressed, ch holds the value, otherwise equals EOF
    fcntl(STDIN_FILENO, F_SETFL, oldf);              // Restore normal behaviour

    // If a key was pressed
    if (ch != EOF) {
        ungetc(ch, stdin); // Feed obtained input into actual input
        return 1;          // Return true
    }

    return 0; // Return false
}
