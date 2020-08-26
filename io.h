
#ifndef __S_TALK_IO__
#define __S_TALK_IO__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

// thread functions for handling input from keyboard
// and displaying writing messages to user respectively
void* input(void *args);
void* output(void *args);

#endif // __S_TALK_IO__