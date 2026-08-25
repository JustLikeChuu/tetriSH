#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "lib/libhtttp.h"
#include "utils/logger.h"

// array of accepted methods
const MethodMapping HTTTP_METHODS[] = {
    {"MOVE", REQ_MOVE},
    {"DROP", REQ_DROP},
    {"ROTATE", REQ_ROTATE},
    {"STATE", REQ_STATE},
    {"ATTACK", REQ_ATTACK},
    {"ACTION", REQ_ACTION},
    {"ROSTER", REQ_ROSTER}};
const int N_HTTTP_METHODS = sizeof(HTTTP_METHODS) / sizeof(MethodMapping);

// HTTTP header fields
const char* htttp_headers[] = {
    "Content-Length",
    "Content-Type",
    "Player-Id",
    "Date"};
const int N_HTTTP_HEADERS = sizeof(htttp_headers) / sizeof(char*);

/*
Function generates date
*/
void get_date_str(char* buf)
{
    time_t now = time(NULL);
    struct tm tm_utc;
    gmtime_r(&now, &tm_utc);
    strftime(buf, 30, "%a, %d %b %Y %H:%M:%S GMT", &tm_utc);
}

/*
Function finds the correct HTTTP method
Returns enum value if found, else returns UNKOWN=0
*/
MethodHTTTP string_to_method(char* s)
{
    // case where string is empty
    if (!s) {
        LOG_E("[string_to_method()] string not found");
        return UNKNOWN;
    }

    // iterate until matching string
    for (int i = 0; i < N_HTTTP_METHODS; i++) {
        // matching string in legal methods
        if (strcmp(s, HTTTP_METHODS[i].string) == 0) {
            LOG_I("[string_to_method()] string \"%s\" maps to method %d", s, HTTTP_METHODS[i].method);
            return HTTTP_METHODS[i].method;
        }
    }

    // catch if none found
    LOG_I("[string_to_method()] string could not be mapped to any method, returning UNKOWN = 0");
    return UNKNOWN;
}

/*
Simple boolean function to determine if method is legal
*/
bool is_legal_method(char* s)
{
    return (string_to_method(s) != UNKNOWN);
}

/*
Function returns the string corresponding to method enum value
*/
char* method_to_string(MethodHTTTP v)
{
    // case where string is empty
    if (v == UNKNOWN) {
        LOG_E("[string_to_method()] string not found");
        return NULL;
    }

    // iterate until matching string
    for (int i = 0; i < N_HTTTP_METHODS; i++) {
        // matching string in legal methods
        if (v == HTTTP_METHODS[i].method) {
            return HTTTP_METHODS[i].string;
        }
    }

    LOG_E("[method_to_string()] method %d does not map to any known string", v);
    return NULL;
}

/*
Returns if header is legal
*/
bool is_legal_header_field(char* s)
{
    // case where string is empty
    if (!s) {
        LOG_E("[is_legal_header_field()] string not found");
        return false;
    }

    // iterate until matching string
    for (int i = 0; i < N_HTTTP_HEADERS; i++) {
        // matching string in legal headers
        if (strcmp(s, htttp_headers[i]) == 0) {
            LOG_I("[is_legal_header_field()] string \"%s\" is a header", s);
            return true;
        }
    }

    return false;
}

void req_create_action(uint32_t id, MethodHTTTP method, InputPayload* payload, ParsedMsgHT* formatted_msg)
{
    // request line
    formatted_msg->token1 = method_to_string(method);
    char* path = malloc(MAX_BUF);
    sprintf(path, "/room/%u/player/%u", 0, id); // room not implemented
    formatted_msg->token2 = path;
    formatted_msg->token3 = CURRENT_VER;

    // headers
    char* id_str = malloc(16);
    sprintf(id_str, "%u", id);
    msg_add_header(formatted_msg, "Player-Id", id_str);
    msg_add_header(formatted_msg, "Content-Type", "application/input-payload");
    char* date_str = malloc(30);
    get_date_str(date_str);
    msg_add_header(formatted_msg, "Date", date_str);

    // body
    char* body = malloc(MAX_BUF);
    payload_encode_input(body, payload);
    msg_add_body(formatted_msg, body);
    char* content_len = malloc(16);
    sprintf(content_len, "%u", (unsigned)strlen(body));
    msg_add_header(formatted_msg, "Content-Length", content_len);
}

void req_create_state(uint32_t id, StatePayload* payload, ParsedMsgHT* formatted_msg)
{
    // request line
    formatted_msg->token1 = method_to_string(REQ_STATE);
    char* path = malloc(MAX_BUF);
    sprintf(path, "/room/%u/player/%u", 0, id); // room not implemented
    formatted_msg->token2 = path;
    formatted_msg->token3 = CURRENT_VER;

    // headers
    char* id_str = malloc(16);
    sprintf(id_str, "%u", id);
    msg_add_header(formatted_msg, "Player-Id", id_str);
    msg_add_header(formatted_msg, "Content-Type", "application/state-payload");
    char* date_str = malloc(30);
    get_date_str(date_str);
    msg_add_header(formatted_msg, "Date", date_str);

    // body
    char* body = malloc(MAX_BUF);
    payload_encode_state(body, payload);
    msg_add_body(formatted_msg, body);
    char* content_len = malloc(16);
    sprintf(content_len, "%u", (unsigned)strlen(body));
    msg_add_header(formatted_msg, "Content-Length", content_len);
}

void req_create_attack(uint32_t id, AttackPayload* payload, ParsedMsgHT* formatted_msg)
{
    // request line
    formatted_msg->token1 = method_to_string(REQ_ATTACK);
    char* path = malloc(MAX_BUF);
    sprintf(path, "/room/%u/player/%u", 0, id); // room not implemented
    formatted_msg->token2 = path;
    formatted_msg->token3 = CURRENT_VER;

    // headers
    char* id_str = malloc(16);
    sprintf(id_str, "%u", id);
    msg_add_header(formatted_msg, "Player-Id", id_str);
    msg_add_header(formatted_msg, "Content-Type", "application/state-payload");
    char* date_str = malloc(30);
    get_date_str(date_str);
    msg_add_header(formatted_msg, "Date", date_str);

    // body
    char* body = malloc(MAX_BUF);
    payload_encode_attack(body, payload);
    msg_add_body(formatted_msg, body);
    char* content_len = malloc(16);
    sprintf(content_len, "%u", (unsigned)strlen(body));
    msg_add_header(formatted_msg, "Content-Length", content_len);
}

void req_create_roster(uint32_t id, RosterPayload* payload, ParsedMsgHT* formatted_msg)
{
    // request line
    formatted_msg->token1 = method_to_string(REQ_ROSTER);
    char* path = malloc(MAX_BUF);
    sprintf(path, "/room/%u/player/%u", 0, id); // room not implemented
    formatted_msg->token2 = path;
    formatted_msg->token3 = CURRENT_VER;

    // headers
    char* id_str = malloc(16);
    sprintf(id_str, "%u", id);
    msg_add_header(formatted_msg, "Player-Id", id_str);
    msg_add_header(formatted_msg, "Content-Type", "application/roster-payload");
    char* date_str = malloc(30);
    get_date_str(date_str);
    msg_add_header(formatted_msg, "Date", date_str);

    // body
    char* body = malloc(MAX_BUF);
    payload_encode_roster(body, payload);
    msg_add_body(formatted_msg, body);
    char* content_len = malloc(16);
    sprintf(content_len, "%u", (unsigned)strlen(body));
    msg_add_header(formatted_msg, "Content-Length", content_len);
}

void req_extract_info(ParsedMsgHT* formatted_msg, MethodHTTTP* method, uint32_t* id, char** body)
{
    *method = string_to_method((char*)HT_REQ_METHOD(formatted_msg));

    *id = 0;
    for (int i = 0; i < formatted_msg->n_headers; i++) {
        if (strcmp(formatted_msg->headers[i].field, "Player-Id") == 0) {
            *id = (uint32_t)strtoul(formatted_msg->headers[i].value, NULL, 10);
            break;
        }
    }

    *body = (char*)formatted_msg->body;
}
