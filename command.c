#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMD_BUFFER_SIZE (512)
#define CMD_SIZE (128)
#define CMD_MAX_ARGS (10)

/* Input buffer */
char cmd_buffer[CMD_BUFFER_SIZE]; 

/* Current comand */
char* cmd_args[CMD_SIZE];
int   cmd_arg_count = 0;

int read_command();
void do_begin();
void do_join();
void do_newa();
void do_newb();
void do_newid();
void do_crash();
void do_delay();
void do_commit();
void do_commit_crash();
void do_abort();
void do_abort_crash();
void do_vote_abort();

int main(int argc, char* argv[])
{
    int i = 0;
    for(i = 0; i < CMD_MAX_ARGS; i++) 
        cmd_args[i] = malloc(CMD_SIZE);

    while(1) 
    {
        read_command();
        if (cmd_arg_count == 0)
            continue;

        if (strncmp(cmd_args[0], "begin", 5) == 0) {
            do_begin();
            continue;
        }
        if (strncmp(cmd_args[0], "join", 4) == 0) {
            do_join();
            continue;
        }
        if (strncmp(cmd_args[0], "newa", 4) == 0) {
            do_newa();
            continue;
        }
        if (strncmp(cmd_args[0], "newb", 4) == 0) {
            do_newb();
            continue;
        }
        if (strncmp(cmd_args[0], "newid", 5) == 0) {
            do_newid();
            continue;
        }
        if (strncmp(cmd_args[0], "crash", 5) == 0) {
            do_crash();
            continue;
        }
        if (strncmp(cmd_args[0], "delay", 5) == 0) {
            do_delay();
            continue;
        }
        if (strncmp(cmd_args[0], "commit", 6) == 0) {
            do_commit();
            continue;
        }
        if (strncmp(cmd_args[0], "commitcrash", 11) == 0) {
            do_commit_crash();
            continue;
        }
        if (strncmp(cmd_args[0], "abort", 5) == 0) {
            do_abort();
            continue;
        }
        if (strncmp(cmd_args[0], "abortcrash", 10) == 0) {
            do_abort_crash();
            continue;
        }
        if (strncmp(cmd_args[0], "voteabort", 9) == 0) {
            do_vote_abort();
            continue;
        }

        printf("Unknown command '%s'\n", cmd_args[0]);
    }

    return 0;
}

int read_command() 
{
    printf("> ");

    char* str = fgets(cmd_buffer, CMD_BUFFER_SIZE, stdin);
    if (str == NULL) {
        printf("Nothing read\n");
        exit(-1);
    }

    int i;
    for(i = 0; i < CMD_BUFFER_SIZE; i++) {
        if (str[i] == '\n')
            str[i] = '\0';
        if (str[i] == '\0')
            break;
    }

    /* split args */
    int last = 0;
    int arg = 0;
    int len = 0;
    for(i = 0; i < CMD_BUFFER_SIZE && str[i] != '\0'; i++) 
    {
        if (arg >= CMD_MAX_ARGS) {
            printf("Maximum arguments reached\n");
            break;
        }

        if (str[i] != ' ')
            continue;

        len = i - last;
        if (len <= 0)
            continue;

        strncpy(cmd_args[arg], &str[last], len);
        cmd_args[arg][len] = '\0';

        last = i + 1;
        arg++;
    }

    len = i - last;
    if (len > 0) {
        strncpy(cmd_args[arg], &str[last], len);
        cmd_args[arg][len] = '\0';
        arg++;
    }

    cmd_arg_count = arg;

    return 1;
}

void do_begin() {
    if (cmd_arg_count != 6) {
        printf("usage: begin [worker_host] [worker_port] [tm_host] [tm_port] [tid]\n");
        return;
    }
}

void do_join() { 
    if (cmd_arg_count != 6) {
        printf("usage: join [worker_host] [worker_port] [tm_host] [tm_port] [tid]\n");
        return;
    }
}

void do_newa() { 
    if (cmd_arg_count != 4) {
        printf("usage: newa [worker_host] [worker_port] [new value]\n");
        return;
    }
}

void do_newb() { 
    if (cmd_arg_count != 4) {
        printf("usage: newb [worker_host] [worker_port] [new value]\n");
        return;
    }
}

void do_newid() { 
    if (cmd_arg_count != 4) {
        printf("usage: newid [worker_host] [worker_port] [new value]\n");
        return;
    }
}

void do_crash() { 
}

void do_delay() { 
}

void do_commit() { 
}

void do_commit_crash() { 
}

void do_abort() { 
}

void do_abort_crash() { 
}

void do_vote_abort() { 
}
