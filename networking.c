#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "networking.h"

#define HANDLE_BAD_NET(res) \
    if (NET_FAIL == res)    \
        PRINT_NET_ERR(res);

void* rx(void *args)
{
    Arg_t *rxArgs = (Arg_t *)args;
    struct addrinfo *info = *(rxArgs->net.addrInfo);

    int res = bind(rxArgs->net.sockFD, info->ai_addr, info->ai_addrlen);
    HANDLE_BAD_NET(res);

    struct sockaddr_storage farAddr;
    socklen_t farLen = sizeof(farAddr);

    int bytesRecv;
    char recvBuf[MAX_MSG_LEN];

    while ((bytesRecv = recvfrom(rxArgs->net.sockFD, recvBuf, MAX_MSG_LEN, 0,
           (struct sockaddr *)&farAddr, &farLen)) >= 0)
    {
        if (CONCURRENCY_BAD == add_to_list(rxArgs, recvBuf, false)) {
            break;
        }

        memset(recvBuf, 0, MAX_MSG_LEN);
    }

    HANDLE_BAD_NET(bytesRecv);
    return NULL;
}

void* tx(void *args)
{
    Arg_t *txArgs = (Arg_t *)args;
    struct addrinfo *info = *(txArgs->net.addrInfo);

    int bytesSent = 0;
    char msgBuf[MAX_MSG_LEN];

    while ((bytesSent >= 0) &&
            CONCURRENCY_OK == read_from_list(txArgs->list, msgBuf, txArgs->mutex, txArgs->cond))
    {
        bytesSent = sendto(txArgs->net.sockFD, msgBuf, strlen(msgBuf), 0, info->ai_addr, info->ai_addrlen);
        memset(msgBuf, 0, MAX_MSG_LEN);
    }

    HANDLE_BAD_NET(bytesSent);
    return NULL;
}