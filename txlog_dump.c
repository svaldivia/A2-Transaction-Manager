#include <stdio.h>

#include "common.h"
#include "txlog.h"

vclock_t vclock[MAX_NODES];

int main(int argc, char* argv[])
{
    txlog_t* log;
    txlog_open(&log, argv[1]);

    txlog_read_clock(log, vclock);

    printf("Vector clock:\n");
    vclock_dump(vclock);

    int i;
    txlog_entry_t entry;

    for(i = 0; i < log->header->tx_count; i++) {
        txlog_read_entry(log, i, &entry);
        txentry_print(&entry);
    }

    return 0;
}
