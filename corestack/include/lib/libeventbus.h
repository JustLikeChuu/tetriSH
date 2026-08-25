#ifndef EVENT_BUS_H
#define EVENT_BUS_H

/**
 *  Event Bus System to allow events to be listened to/broadcast.
 *
 *  Example Usage:
 *  // Initialise Event Bus
 *  event_bus_init(EVENT_COUNT);
 *
 *  // Listen to events
 *  void on_player_move(void *args) {
 *      // Cast void* to the expected args struct.
 *      PlayerMoveArgs *a = (PlayerMoveArgs *)args;
 *      printf("Player %d moved to (%d, %d)\n", a->player_id, a->x, a->y);
 *  }
 *  event_bus_listen(EVENT_PLAYER_MOVE, on_player_move);
 *
 *  // Trigger Event
 *  PlayerMoveArgs move = { .player_id = 1, .x = 3, .y = 5 };
 *  event_bus_trigger(EVENT_PLAYER_MOVE, &move);
 *
 *  // Stop listening to events
 *  event_bus_stop_listening(EVENT_PLAYER_MOVE, on_player_move);
 *
 *  // Free event bus
 *  event_bus_free();
 */

// Listener function signature
// args is cast to whatever struct the event uses
typedef void (*EventListener)(void* args);

/**
 * @brief Initialise event bus with number of events required.
 * Call this before using the event bus!
 *
 * @param num_events number of events, usually just dedicate the last entry in your enum of events, such as EVENT_COUNT
 */
void event_bus_init(int num_events);

/**
 * @brief Free event bus memory.
 * Call this at the end of your program!
 */
void event_bus_free(void);

/**
 * @brief Start listening for an event
 *
 * @param event_type enum of event type
 * @param listener listener function
 */
void event_bus_listen(int event_type, EventListener listener);

/**
 * @brief Stop listening to event

 * @param event_type enum of event type
 * @param listener listener function
 */
void event_bus_stop_listening(int event_type, EventListener listener);

/**
 * @brief Broadcast trigger of event to all listeners

 * @param event_type enum of event type
 * @param args arguments to provide to listeners
 */
void event_bus_trigger(int event_type, void* args);
#endif
