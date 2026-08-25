#ifndef LIBHT_H
#define LIBHT_H

#define MAX_BUF 2048   // must match libbattleroyale's MSG_CONTENT_LENGTH; brserver/brclient app-msg calls memcpy this many bytes in/out of a HyperText buffer
#define MAX_HEADERS 16 // kinda arbritrary rn

// hypertext separators
#define HT_END_LINE "\r\n"
#define HT_TOKEN_SEP " "
#define HT_HEADER_SEP ": "

// hypertext message structs and aliases
typedef char HyperText[MAX_BUF];

typedef struct {
    const char* field;
    const char* value;
} HeaderHT;

typedef struct {
    const char* token1;
    const char* token2;
    const char* token3;
    int n_headers;
    HeaderHT headers[MAX_HEADERS];
    const char* body;
} ParsedMsgHT;

/* macros to get version, method, path for requests */
#define HT_REQ_METHOD(req) ((req)->token1)
#define HT_REQ_PATH(req) ((req)->token2)
#define HT_REQ_VER(req) ((req)->token3)

/* macros to get version, code, reason for response*/
#define HT_RES_VER(req) ((req)->token1)
#define HT_RES_CODE(req) ((req)->token2)
#define HT_RES_REASON(req) ((req)->token3)

char* next_token(char* s, char* delim);
int parse_hypertext(HyperText ht, ParsedMsgHT* parser_result);

int req_init(ParsedMsgHT* new_req, const char* method, const char* path, const char* ver);
int res_init(ParsedMsgHT* new_res, const char* ver, const char* code, const char* reason);
int msg_add_header(ParsedMsgHT* msg, const char* field, const char* value);
int msg_add_body(ParsedMsgHT* msg, const char* body);

int convert_to_hypertext(ParsedMsgHT* msg, HyperText converted_ht);

#endif
