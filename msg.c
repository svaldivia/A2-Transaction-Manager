#include <assert.h>
#include <string.h>
#include <stdint.h>

#include "msg.h"
#include "common.h"

void message_init(message_t* msg, vclock_t* vclock) 
{
    assert(msg != NULL);

    /* Clear struct */
    memset((void*)msg, 0, sizeof(message_t));

    if (vclock != NULL) {
        /* If we passed a vector clock, copy it to the message */
        int i;
        for(i = 0; i < MAX_NODES; i++) 
            msg->vclock[0] = vclock[0];
    }
}

void message_write_begin(message_t* msg, char* tm_host, uint32_t tm_port, uint32_t transaction) 
{
    assert(tm_host != NULL);
    msg->type = BEGINTX;
    msg->port = tm_port;
    msg->tid  = transaction;
    strcpy(msg->strdata, tm_host);
}

void message_write_join(message_t* msg, char* tm_host, uint32_t tm_port, uint32_t transaction) 
{
    assert(tm_host != NULL);
    msg->type = JOINTX;
    msg->port = tm_port;
    msg->tid  = transaction;
    strcpy(msg->strdata, tm_host);
}

void message_write_new_A(message_t* msg, int32_t new_a) {
    msg->type  = NEW_A;
    msg->value = new_a;
}

void message_write_new_B(message_t* msg, int32_t new_b) {
    msg->type  = NEW_B;
    msg->value = new_b;
}

void message_write_new_ID(message_t* msg, char* id_str) {
    assert(id_str != NULL);
    msg->type  = NEW_IDSTR;
    strcpy(msg->strdata, id_str);
}

void message_write_crash(message_t* msg) {
    msg->type  = CRASH;
}

void message_write_delay(message_t* msg, int32_t delay) {
    msg->type  = DELAY_RESPONSE;
    msg->delay = delay;
}

void message_write_commit(message_t* msg) {
    msg->type = COMMIT;
}

void message_write_commit_crash(message_t* msg) {
    msg->type = COMMIT_CRASH;
}

void message_write_abort(message_t* msg) {
    msg->type = ABORT;
}

void message_write_abort_crash(message_t* msg) {
    msg->type = ABORT_CRASH;
}

void message_write_vote_abort(message_t* msg) {
    msg->type = VOTE_ABORT;
}

void message_to_nbo(message_t* msg) 
{
    msg->type  = htonl(msg->type);
    msg->tid   = htonl(msg->tid);
    msg->port  = htonl(msg->port);
    msg->value = htonl(msg->value);
    msg->delay = htonl(msg->delay);

    int i;
    for(i = 0; i < MAX_NODES; i++) {
        msg->vclock[i].nodeId = htonl(msg->vclock[i].nodeId);
        msg->vclock[i].time   = htonl(msg->vclock[i].time);
    }
}

void message_from_nbo(message_t* msg) 
{
    msg->type  = ntohl(msg->type);
    msg->tid   = ntohl(msg->tid);
    msg->port  = ntohl(msg->port);
    msg->value = ntohl(msg->value);
    msg->delay = ntohl(msg->delay);

    int i;
    for(i = 0; i < MAX_NODES; i++) {
        msg->vclock[i].nodeId = ntohl(msg->vclock[i].nodeId);
        msg->vclock[i].time   = ntohl(msg->vclock[i].time);
    }
}

const char* message_string(message_t* msg) 
{
    switch(msg->type) {
        case BEGINTX:        return "Begin Transaction";
        case JOINTX:         return "Join Transaction";
        case NEW_A:          return "New A";
        case NEW_B:          return "New B";
        case NEW_IDSTR:      return "New ID String";
        case DELAY_RESPONSE: return "Delay Response";
        case COMMIT:         return "Commit";
        case COMMIT_CRASH:   return "Commit Crash";
        case ABORT:          return "Abort";
        case ABORT_CRASH:    return "Abort Crash";
        case VOTE_ABORT:     return "Vote Abort";
        default:             return "Unknown";
    }
}
