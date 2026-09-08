/* 
    this file serves as the fuzzing harness
    
    - an adapter function; sits between libFuzzer's muitation engine (which only knows how to generate raw bytes)
    and the actual code (which expects specific typed structures)
    
    - only job is to take (bytes, length), translate that into whatever shape the real code expects, call the real code,
    and get out of the way
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lib/libhypertext.h"
#include "lib/libhtttp.h"
#include "libtetrisprotocol/protocol.h"

// when compilling with -fsanitize=fuzzer, linker pulls in libFuzzer's own main() which calls LLVMFuzzerTestOneInput 
// in a tight loop, each time handing it a candidate input it generated -> engine drives the harness; no control flow beyond "given this input, what happens"
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) // must follow exact function name and signature!
{
    // out of bounds -> return 0 is the only value currently allowed to return
    if (size == 0 || size >= MAX_BUF) {
        return 0;
    }

    HyperText buf = {0};
    memcpy(buf, data, size);
    buf[size] = '\0'; // parser uses strstr/strlen, needs NUL termination

    // actual code under test
    ParsedMsgHT parsed = {0};
    if (parse_hypertext(buf, &parsed) < 0) {
        return 0;
    }

    MethodHTTTP method;
    uint32_t sender_id;
    char* body;
    req_extract_info(&parsed, &method, &sender_id, &body);

    switch (method) {
        case REQ_STATE: {
            StatePayload incoming;
            payload_decode_state(body, &incoming);
            break;
        }
        case REQ_ROSTER: {
            struct {
                uint32_t count;
                uint32_t ids[MAX_LOBBY_SIZE];
            } incoming_roster = {0};
            payload_decode_roster(body, (RosterPayload*)&incoming_roster);
            break;
        }
        case REQ_ATTACK: {
            AttackPayload incoming;
            payload_decode_attack(body, &incoming);
            break;
        }
        default: {
            InputPayload incoming;
            payload_decode_input(body, &incoming);
            break;
        }
    }

    return 0;
}