#ifndef __S_TALK_COMMON__
#define __S_TALK_COMMON__

#include <errno.h>
#include <pthread.h>

#include "list.h"

#define MAX_MSG_LEN 1024
#define THREAD_COUNT 4

#define CONCURRENCY_OK 0
#define CONCURRENCY_BAD -1

#define TERM_TYPED "!\n"
#define TERM_FILE_NL "\n!\n"
#define TERM_FILE_NULL "\n!\0"

#define __LOCK(l, ret)                                             \
    if (pthread_mutex_lock(l) != CONCURRENCY_OK) {                 \
        printf("%s: mutex lock error: %d\n", __FUNCTION__, errno); \
        return ret;                                                \
    }

#define __UNLOCK(l, ret)                                             \
    if (pthread_mutex_unlock(l) != CONCURRENCY_OK) {                 \
        printf("%s: mutex unlock error: %d\n", __FUNCTION__, errno); \
        return ret;                                                  \
    }

#define __WAIT(c, l, ret)                                              \
    if (pthread_cond_wait(c, l) != CONCURRENCY_OK) {                   \
        printf("%s: condition wait error: %d\n", __FUNCTION__, errno); \
        return ret;                                                    \
    }

#define __SIGNAL(c, ret)                                                 \
    if (pthread_cond_signal(c) != CONCURRENCY_OK) {                      \
        printf("%s: condition signal error: %d\n", __FUNCTION__, errno); \
        return ret;                                                      \
    }

// thread argument structure
// net is optionally filled
typedef struct Arg_t {
    pthread_cond_t* cond;
    pthread_mutex_t* mutex;
    List* list;
    pthread_t* threads[THREAD_COUNT];

    struct {
        int sockFD;
        struct addrinfo** addrInfo;
    } net;
} Arg_t;

// thread-safe call to List_append() and List_remove() (from the head)
// blocking if list in use or cannot currently support addition of new message
// will handle shutdown of program if terminating sequence encountered
// returns CONCURRENCY_OK/CONCURRENCY_BAD on success/failure
int add_to_list(Arg_t *args, char *buf, bool sendTerm);
int read_from_list(List *list, char *buf, pthread_mutex_t *mutex, pthread_cond_t *cond);

#endif // __S_TALK_COMMON__
