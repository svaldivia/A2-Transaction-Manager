#include <stdio.h>

#include "common.h"
#include "txlog.h"

vclock_t vclock[MAX_NODES];

int main(int argc, char* argv[])
{
    txlog_t* log;
    txlog_open(&log, "test_log.dat");

    txlog_read_clock(log, vclock);

    printf("Vector clock:\n");
    vclock_dump(vclock);

    int i;
    txlog_entry_t entry;

    for(i = 0; i < log->header->tx_count; i++) {
        txlog_read_entry(log, i, &entry);
        printf("log entry type: %d\n", entry.type);
    }

    entry.type = LOG_BEGIN;
    txlog_append(log, &entry);

    return 0;
}
