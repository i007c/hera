
#ifndef __HERA_COMMON_H__
#define __HERA_COMMON_H__


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#include <assert.h>
#include <math.h>
#include <time.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

#include <signal.h>
#include <endian.h>

#include <sys/random.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

#if defined(__GNUC__) || defined(__clang__)
    #define UNUSED(name) _unused_ ## name __attribute__((unused))
#else
    #define UNUSED(name) _unused_ ## name
#endif

#define LEN(x) sizeof(x) / sizeof(x[0])

typedef enum status_t {
    STS_SUCCESS = 0,
    // STS_BAD_ID = 4001,
    // STS_BAD_INDEX = 4002,
    // STS_FORBIDDEN = 4003,
    // STS_DB_ERROR = 5001,
    // STS_BAD_REQUEST_ARGS = 7001,
} status_t;

#endif //__HERA_COMMON_H__

