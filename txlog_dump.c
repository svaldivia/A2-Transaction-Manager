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

    return 0;
}
