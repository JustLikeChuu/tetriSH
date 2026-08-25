#ifndef LIBHTTTP_H
#define LIBHTTTP_H

#include "libhypertext.h"
#include "libhtttp/payload.h"

#define CURRENT_VER "HTTTP/1.0"

// HTTTP methods
typedef enum {
    UNKNOWN = 0,
    REQ_MOVE,
    REQ_DROP,
    REQ_ROTATE,
    REQ_STATE,
    REQ_ATTACK,
    REQ_ACTION,
    REQ_ROSTER
} MethodHTTTP;

// mapping to string
typedef struct {
    char* string;
    MethodHTTTP method;
} MethodMapping;

// array of accepted methods
extern const MethodMapping HTTTP_METHODS[];
extern const int N_HTTTP_METHODS;

// HTTTP header fields
extern const char* htttp_headers[];
extern const int N_HTTTP_HEADERS;

// important request builders
void req_create_action(uint32_t id, MethodHTTTP method, InputPayload* payload, ParsedMsgHT* formatted_msg);
void req_create_state(uint32_t id, StatePayload* payload, ParsedMsgHT* formatted_msg);
void req_create_attack(uint32_t id, AttackPayload* payload, ParsedMsgHT* formatted_msg);
void req_create_roster(uint32_t id, RosterPayload* payload, ParsedMsgHT* formatted_msg);

// parses an incoming request, pulling out the method, the Player-Id header, and the body
void req_extract_info(ParsedMsgHT* formatted_msg, MethodHTTTP* method, uint32_t* id, char** body);

#endif
