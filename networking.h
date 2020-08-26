#ifndef __S_TALK_NETWORKING__
#define __S_TALK_NETWORKING__

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

#include "common.h"
#include "list.h"

#define NET_FAIL -1
#define MAX_QD_REQS 1

#define PRINT_NET_ERR(res) printf("%s: errno: %d, msg: %s\n", __FUNCTION__, errno, gai_strerror(res));

void* rx(void *args);
void* tx(void *args);

#endif // __S_TALK_NETWORKING__
