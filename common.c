#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#define LOCK(l) __LOCK(l, CONCURRENCY_BAD);
#define UNLOCK(l) __UNLOCK(l, CONCURRENCY_BAD);
#define WAIT(c, l) __WAIT(c, l, CONCURRENCY_BAD);
#define SIGNAL(c) __SIGNAL(c, CONCURRENCY_BAD);

#define ADD_TO_LIST(arg, b)                                                    \
    if (CONCURRENCY_BAD == __add_to_list(arg->list, b, arg->mutex, arg->cond)) \
        return CONCURRENCY_BAD;

// cancel all threads pointed to in threads
// exit poorly on error
static void __cancel_threads(pthread_t **threads)
{
    for (int i=0; i<THREAD_COUNT; i++) {
            if (CONCURRENCY_BAD == pthread_cancel(*(threads[i]))) {
                printf("%s: failed to cancel a thread... exiting without cleanup\n", __FUNCTION__);
                exit(-1);
            }
        }
}

// check if msg is the terminating sequence or contains it
// removes terminating sequence from msg if it exists so it can be sent
// returns CONCURRENCY_BAD if term sequence found, CONCURRENCY_OK otherwise
static int __check_terminate(char *msg)
{
    char *substr = NULL;
    char *substr1 = NULL;

    // check if we have a typed terminating char
    int res = strcmp(msg, TERM_TYPED);

    // two possibilities for terminating char in a file
    // ignore the result that we confirm to be NULL
    substr = strstr(msg, TERM_FILE_NL);
    substr1 = strstr(msg, TERM_FILE_NULL);
    substr = substr ? substr : substr1;

    if (!res || substr) {
        if (substr) {
            memset(substr, 0, MAX_MSG_LEN - (substr - msg)); // remove term sequence
            strcat(substr, "\n"); // add newline to the end
        }

        return CONCURRENCY_BAD;
    }

    return CONCURRENCY_OK;
}

static int __add_to_list(List *list, char *msg, pthread_mutex_t *mutex, pthread_cond_t *cond)
{
    char *newMsg = calloc(MAX_MSG_LEN, sizeof(char));
    strcpy(newMsg, msg);

    LOCK(mutex);
    {
        while (LIST_FAIL == List_append(list, newMsg)) {
            // might get cancelled while blocked, free memory in case and reallocate when running
            // since it's not in list it won't be handled during cleanup
            free(newMsg);
            WAIT(cond, mutex);

            newMsg = calloc(MAX_MSG_LEN, sizeof(char));
            strcpy(newMsg, msg);
        }
    }
    UNLOCK(mutex);

    SIGNAL(cond);
    return CONCURRENCY_OK;
}

int add_to_list(Arg_t *args, char *buf, bool sendTerm)
{
    int res = __check_terminate(buf);

    // check to see if buf non-empty after __check_terminate call
    if (strlen(buf) > 0) {
        ADD_TO_LIST(args, buf);
    }

    if (res == CONCURRENCY_BAD) {
        if (sendTerm) {
            ADD_TO_LIST(args, TERM_TYPED); // add terminating char sequence to list
        }

        LOCK(args->mutex);
        {
            while (List_count(args->list) > 0) {
                WAIT(args->cond, args->mutex);
            }

            __cancel_threads(args->threads);
        }
        UNLOCK(args->mutex);
    }

    return CONCURRENCY_OK;
}

int read_from_list(List *list, char *buf, pthread_mutex_t *mutex, pthread_cond_t *cond)
{
    char *msg;

    LOCK(mutex);
    {
        while (List_count(list) < 1) {
            WAIT(cond, mutex);
        }

        List_first(list);        // move cursor to head since it will be at tail
        msg = List_remove(list); // copy and remove msg from list
    }
    UNLOCK(mutex);

    SIGNAL(cond);

    strcpy(buf, msg);
    free(msg);
    return CONCURRENCY_OK;
}