#ifndef MSG_H
#define MSG_H 1

#include <stdint.h>
#include "common.h"

#define STRDATA_LEN (64)

enum cmdType {
    BEGINTX = 1000,
	JOINTX,
	NEW_A,
	NEW_B,
	NEW_IDSTR,
	DELAY_RESPONSE,
	CRASH,
	COMMIT,
	COMMIT_CRASH,
	ABORT, 
    ABORT_CRASH, 
	VOTE_ABORT
};

typedef enum cmdType cmdType;

typedef struct 
{
  cmdType    type;      
  uint32_t   tid;        // Transaction ID
  uint32_t   port;
  int32_t    value;   // New value for A or B
  int32_t    delay;
  char       strdata[STRDATA_LEN];
  vclock_t   vclock[MAX_NODES];
} message_t;

/* Initializes a message and writes a vector clock to it. Should be called before any message_write_x function */
void message_init(message_t* msg, vclock_t* vclock);

void message_write_begin(message_t* msg, char* tm_host, uint32_t tm_port, uint32_t transaction);
void message_write_join(message_t* msg, char* tm_host, uint32_t tm_port, uint32_t transaction);
void message_write_new_A(message_t* msg, int32_t new_a);
void message_write_new_B(message_t* msg, int32_t new_b); 
void message_write_new_ID(message_t* msg, char* id_str);
void message_write_crash(message_t* msg); 
void message_write_delay(message_t* msg, int32_t delay);
void message_write_commit(message_t* msg); 
void message_write_commit_crash(message_t* msg); 
void message_write_abort(message_t* msg); 
void message_write_abort_crash(message_t* msg); 
void message_write_vote_abort(message_t* msg); 

void message_to_nbo(message_t* msg);
void message_from_nbo(message_t* msg);

#endif 
