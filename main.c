#include <errno.h>
#include <stdio.h>

#include "io.h"
#include "list.h"
#include "networking.h"

#define NUM_ARGS 3

#define HANDLE_ERROR(res, l)                                                      \
    if (res == -1) {                                                              \
        printf("%s: error (%d) occurred in main() of s-talk\nexiting...\n", \
                __FUNCTION__, errno);                                             \
        goto l;                                                                   \
    }

/* Concurrency */
static pthread_cond_t rxCond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t txCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t rxMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t txMutex = PTHREAD_MUTEX_INITIALIZER;

/* Networking */
static struct addrinfo *rxRes, *txRes;

/* thread arguments */
static Arg_t inputArgs, outputArgs;
static Arg_t rxArgs, txArgs;

// initialize a socket with error checking
// returns result of socket() and fills resInfo
static int init_socket(struct addrinfo **resInfo, char *host, char *port) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    static int socketFD = CONCURRENCY_BAD; // only want one socket

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if (!host) {
        hints.ai_flags = AI_PASSIVE;
    }

    int res = getaddrinfo(host, port, &hints, resInfo);

    if (res != 0 || !(*resInfo)) {
        PRINT_NET_ERR(res);
        exit(EXIT_FAILURE);
    }

    if (CONCURRENCY_BAD == socketFD) {
        socketFD = socket((*resInfo)->ai_family, (*resInfo)->ai_socktype, (*resInfo)->ai_protocol);
    }

    return socketFD;
}

int main(int argc, char *argv[])
{
    if (argc != NUM_ARGS + 1) {
        printf("Need %d arguments...\n", NUM_ARGS);
        exit(EXIT_FAILURE);
    }

    /* initialize socket connections */
    int rxSockFD = init_socket(&rxRes, NULL, argv[1]);
    int txSockFD = init_socket(&txRes, argv[2], argv[3]);

    pthread_t inputThread, outputThread, rxThread, txThread;
    pthread_t* threadPtrs[THREAD_COUNT] = { &inputThread, &outputThread, &rxThread, &txThread };

    List *rxList = List_create();
    List *txList = List_create();

    /* build thread arguments */
    inputArgs.mutex = &txMutex;
    inputArgs.cond = &txCond;
    inputArgs.list = txList;
    memcpy(inputArgs.threads, threadPtrs, THREAD_COUNT*sizeof(pthread_t *));

    outputArgs.mutex = &rxMutex;
    outputArgs.cond = &rxCond;
    outputArgs.list = rxList;
    memcpy(outputArgs.threads, threadPtrs, THREAD_COUNT*sizeof(pthread_t *));

    rxArgs.mutex = &rxMutex;
    rxArgs.cond = &rxCond;
    rxArgs.list = rxList;
    rxArgs.net.sockFD = rxSockFD;
    rxArgs.net.addrInfo = &rxRes;
    memcpy(rxArgs.threads, threadPtrs, THREAD_COUNT*sizeof(pthread_t *));

    txArgs.mutex = &txMutex;
    txArgs.cond = &txCond;
    txArgs.list = txList;
    txArgs.net.sockFD = txSockFD;
    txArgs.net.addrInfo = &txRes;
    memcpy(txArgs.threads, threadPtrs, THREAD_COUNT*sizeof(pthread_t *));

    HANDLE_ERROR(pthread_create(&inputThread, NULL, input, (void *)&inputArgs), cleanup);
    HANDLE_ERROR(pthread_create(&outputThread, NULL, output, (void *)&outputArgs), cleanup);
    HANDLE_ERROR(pthread_create(&rxThread, NULL, rx, (void *)&rxArgs), cleanup);
    HANDLE_ERROR(pthread_create(&txThread, NULL, tx, (void *)&txArgs), cleanup);

    HANDLE_ERROR(pthread_join(inputThread, NULL), cleanup);
    HANDLE_ERROR(pthread_join(outputThread, NULL), cleanup);
    HANDLE_ERROR(pthread_join(rxThread, NULL), cleanup);
    HANDLE_ERROR(pthread_join(txThread, NULL), cleanup);

cleanup:
    freeaddrinfo(rxRes);
    freeaddrinfo(txRes);

    List_free(rxList, free);
    List_free(txList, free);

    HANDLE_ERROR(close(rxSockFD), done);

    HANDLE_ERROR(pthread_mutex_destroy(&rxMutex), done);
    HANDLE_ERROR(pthread_mutex_destroy(&txMutex), done);
    HANDLE_ERROR(pthread_cond_destroy(&rxCond), done);
    HANDLE_ERROR(pthread_cond_destroy(&txCond), done);

done:
    return 0;
}