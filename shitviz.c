#include "shitviz.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

FILE* _SHITVIZ_FD = NULL;

void shitviz_append(uint32_t id, char* msg, vclock_t* vclock) 
{
    if (!_SHITVIZ_FD) {
        char name_buffer[128];
        sprintf(name_buffer, "ShiViz_%d.dat", id);
        _SHITVIZ_FD = fopen(name_buffer, "w");

        /* write shitviz header */
    }
    assert(_SHITVIZ_FD);

    /* print message */
    fprintf(_SHITVIZ_FD, "%s\n", msg);
    fprintf(stdout, "ShiViz: %s\n", msg);

    fprintf(_SHITVIZ_FD, "N%d { ", id);

    /* dump nodes */
    int i;
    for(i = 0; i < MAX_NODES; i++) 
    {
        vclock_t* node_clock = &vclock[i];
        if (node_clock->nodeId == 0 || node_clock->time == 0)
            continue;

        if (i != 0)
            fprintf(_SHITVIZ_FD, ", ");
        fprintf(_SHITVIZ_FD, "\"N%d\": %d", node_clock->nodeId, node_clock->time);
    }

    fprintf(_SHITVIZ_FD, " }\n");
    fflush(_SHITVIZ_FD);
}
