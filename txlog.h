#ifndef TXLOG_H
#define TXLOG_H

#include <stdint.h>

#include "common.h"

enum logEntryType 
{
    LOG_BEGIN = 0x10,
    LOG_COMMIT, 
    LOG_ABORT,      

    LOG_PREPARED,
    LOG_UPDATE,
};

typedef enum logEntryType logEntryType;
typedef struct txlog_t txlog_t;
typedef struct txlog_head_t txlog_head_t;
typedef struct txlog_entry_t txlog_entry_t;

struct txlog_t {
    int             file;       /* file descriptor */
    txlog_head_t*   header;     /* pointer to mapped memory */
};

struct txlog_head_t {
    vclock_t        vclock[MAX_NODES];  /* vector clock */
    uint32_t        tx_count;           /* entry count */
    uint32_t        tm_listen_port;
    uint32_t        tm_port;
    char            tm_host[64];
};

struct txlog_entry_t 
{
    logEntryType    type;
    uint32_t        transaction;

    uint32_t        old_a;
    uint32_t        old_b;
    char            old_id[IDLEN];

    vclock_t        vclock[MAX_NODES];
};

void txlog_open(txlog_t** log_ptr, const char* logFileName);
void txlog_close(txlog_t** log_ptr);
void txlog_write_clock(txlog_t* txlog, vclock_t* vclock);
void txlog_read_clock(txlog_t* txlog, vclock_t* out_clock);
void txlog_append(txlog_t* txlog, txlog_entry_t* entry);
void txlog_read_entry(txlog_t* txlog, uint32_t idx, txlog_entry_t* out_entry);

void txentry_init(txlog_entry_t* entry, logEntryType type, uint32_t tid);

#endif
