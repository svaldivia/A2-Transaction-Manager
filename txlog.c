#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

#include "txlog.h"

void txlog_sync(txlog_t*);

/** open (or create) the transaction log file */
void txlog_open(txlog_t** log_ptr, const char* logFileName)
{
    assert(log_ptr);
    assert(logFileName);

    txlog_t* log = malloc(sizeof(txlog_t));
    assert(log);

    *log_ptr = log;

    int fd = 0;
    txlog_head_t* header = NULL;

    fd = open(logFileName, O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR );
    if (fd < 0) {
        printf("Could not open transaction log file '%s'\n", logFileName);
        exit(-1);
    }

    log->file = fd;

    struct stat fstatus;
    if(fstat(fd, &fstatus) < 0) {
        printf("Filestat failed");
        exit(-2);
    }

    if (fstatus.st_size < sizeof(txlog_entry_t)) 
    {
        /* File hasn's been mapped in before so we need to make sure there is enough
        space used in the file to hold the data.  */
        txlog_entry_t space;
        memset(&space, 0, sizeof(txlog_entry_t));

        int rv = write(fd, &space, sizeof(txlog_entry_t));
        if (rv != sizeof(txlog_entry_t)) {
            printf("Some sort of writing error\n");
            exit(-3);
        }
        printf("Transaction log file created\n");
    }

    /* Map to memory */
    header = (txlog_head_t*)mmap(NULL, sizeof(txlog_head_t), PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);

    lseek(log->file, sizeof(txlog_head_t), SEEK_SET);

    if (header == NULL) {
        printf("Log header could not be mapped to memory\n");
        exit(-1);
    }

    log->header = header;
}

/** close transaction log */
void txlog_close(txlog_t** log_ptr) 
{
    txlog_t* log = *log_ptr;

    /* make sure to sync changes if any */
    txlog_sync(log);

    /* unmap header from memory */
    int rv = munmap(log, sizeof(txlog_head_t));
    if (rv < 0) {
        printf("Unmap failed");
        exit(-1);
    }

    /* close log */
    close(log->file);

    /* free allocated memory */
    free(log);
    *log_ptr = NULL;
}

/** flushes the log header to disk */ 
void txlog_sync(txlog_t* log) 
{
    int r = msync(log->header, sizeof(txlog_head_t), MS_SYNC);
    if (r != 0) {
        printf("Unable to sync transaction log\n");
        exit(-1);
    }
}

/** copies a vector clock from the given pointer to the log file */
void txlog_write_clock(txlog_t* log, vclock_t* clock) {
    memcpy(log->header->vclock, clock, MAX_NODES * sizeof(vclock_t));
    txlog_sync(log);
}

/** reads a vector clock from the log file to the given pointer */
void txlog_read_clock(txlog_t* log, vclock_t* out_clock) {
    memcpy(out_clock, log->header->vclock, MAX_NODES * sizeof(vclock_t));
}

/** appends a log entry to the log file */
void txlog_append(txlog_t* log, txlog_entry_t* entry) 
{
    off_t offset = sizeof(txlog_head_t) + log->header->tx_count * sizeof(txlog_entry_t);
    lseek(log->file, offset, SEEK_SET);
    write(log->file, entry, sizeof(txlog_entry_t));

    /* increment number of transactions */
    log->header->tx_count++;

    /* update vector clock & sync */
    txlog_write_clock(log, entry->vclock);
}

/** reads a log entry with the given index */
void txlog_read_entry(txlog_t* log, uint32_t idx, txlog_entry_t* out_entry) 
{
    off_t offset = sizeof(txlog_head_t) + idx * sizeof(txlog_entry_t);
    lseek(log->file, offset, SEEK_SET);
    read(log->file, out_entry, sizeof(txlog_entry_t));
}

void txentry_init(txlog_entry_t* entry, logEntryType type, uint32_t tid, vclock_t* vclock) {
    memset(entry, 0, sizeof(txlog_entry_t));
    memcpy(entry->vclock, vclock, MAX_NODES * sizeof(vclock_t));
    entry->type = type;
    entry->transaction = tid;
}

const char* txentry_type_string(logEntryType type) {
    switch(type) {
        case LOG_BEGIN:     return "BEGIN";
        case LOG_COMMIT:    return "COMMIT";
        case LOG_ABORT:     return "ABORT";
        case LOG_PREPARED:  return "PREPARED";
        case LOG_UPDATE:    return "UPDATE";

        default: return "Unknown";
    }
}

void txentry_print(txlog_entry_t* entry) 
{
    printf("Log Entry: %s\n", txentry_type_string(entry->type));
    switch(entry->type) {
        case LOG_COMMIT:
        case LOG_UPDATE:
            printf("  A  = %d\n", entry->old_a);
            printf("  B  = %d\n", entry->old_b);
            printf("  ID = '%s'\n", entry->old_id);
            break;
        default: break;
    }
}
