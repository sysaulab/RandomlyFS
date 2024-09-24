#ifndef LIBSEEDY_COMMON_H_
#define LIBSEEDY_COMMON_H_

/*****************************************************************************
 *                                                                           *
 *   pstdint.h STANDARD INT SUPPORT AND SHIMS.                               *
 *   http://www.azillionmonkeys.com/qed/pstdint.h                            *
 *                                                                           *
 *****************************************************************************/

#include "pstdint.h"

/* 
 *   16 bit support is for edge cases only.
 *   Please enable it manually
 *   
 *   64 bit support is detected by testing a macro from the pstdint.h file.
 */

/* #define LIBSEEDY_OPT16 */

#define LIBSEEDY_OPT32
/* #define LIBSEEDY_OPT_PROGRESS */

#if defined(UINT64_MAX)
#   define LIBSEEDY_OPT64
#endif

#if defined(_WIN32)
#   include <windows.h>
#   define wait_ms(ms) Sleep(ms)
#   define wait_us(ms) Sleep(1)
#elif defined(__HAIKU__)
#   include <kernel/OS.h>
#   include <cstdlib>
#   include <unistd.h>
#   define wait_ms(ms) usleep(ms * 1000)
#   define wait_us(us) usleep(us)
#else
#   include <pthread.h>
#   include <unistd.h>
#   define wait_ms(ms) usleep(ms * 1000)
#   define wait_us(us) usleep(us)
#endif

#if defined(LIBSEEDY_OPT_PROGRESS)
#   include <stdio.h>       /* For reporting progress when fill takes long time */
#   include <time.h>        /* For reporting progress when fill takes long time */
#endif

#define SEEDY_INIT_MS_ 1
#define SEEDY_INTERVAL_US_ 10
#define NOW() (clock()/(CLOCKS_PER_SEC))

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#define ROT16(a,n) ((a<<(n%16))|(a>>(16-(n%16))))
#define ROT32(a,n) ((a<<(n%32))|(a>>(32-(n%32))))
#define ROT64(a,n) ((a<<(n%64))|(a>>(64-(n%64))))

#endif

