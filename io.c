#include "io.h"
#include "common.h"
#include "list.h"

void* input(void *args)
{
    Arg_t *ioArgs = (Arg_t *)args;
    char msgBuf[MAX_MSG_LEN];
    memset(msgBuf, 0, MAX_MSG_LEN);

    while (CONCURRENCY_BAD != read(STDIN_FILENO, msgBuf, MAX_MSG_LEN)) {
        if (CONCURRENCY_BAD == add_to_list(ioArgs, msgBuf, true)) {
            break;
        }

        memset(msgBuf, 0, MAX_MSG_LEN);
    }

    return NULL;
}

void* output(void *args)
{
    Arg_t *ioArgs = (Arg_t *)args;
    char msgBuf[MAX_MSG_LEN];

    while (CONCURRENCY_OK == read_from_list(ioArgs->list, msgBuf, ioArgs->mutex, ioArgs->cond)) {
        write(STDOUT_FILENO, msgBuf, strlen(msgBuf));
        // clear out buffer in case length of new msg smaller than last
        memset(msgBuf, 0, MAX_MSG_LEN);
    }

    return NULL;
}