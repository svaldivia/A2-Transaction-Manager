#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "msg.h"
#include "server.h"
#include "tworker.h"

void tx_manager_spawn(worker_state_t* state, const char* tm_host, uint32_t tm_port) 
{
    state->is_active = true;
    state->server = NULL;
    state->tm_port = tm_port;
    strcpy(state->tm_host, tm_host);

    /* Set up server :: TMananger*/
    server_alloc(&state->server, 0, 10);
    server_listen(state->server);

    pthread_t thread;
    pthread_create(&thread, NULL, tx_worker_thread, (void*)state); 
}

void* tx_worker_thread(void* params) 
{
    worker_state_t* wstate = (worker_state_t*)params;

    printf("Started TM thread: (%s:%d)\n", wstate->tm_host, wstate->tm_port);

    struct sockaddr_in recv_addr;
    message_t msg;

    while(1) {
        server_recv(wstate->server,&msg,&recv_addr);
        
        switch(msg.type){
        
        }
    }
    
    /* clean up */
    printf("Exiting transaction thread\n");
    server_shutdown(&wstate->server);
    wstate->is_active = false;
    return NULL;
}
