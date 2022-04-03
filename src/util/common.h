#ifndef COMMON_H
#define COMMON_H

// TODO: Add performance and correctness test macros here
/******************************************************************************
 * MACROS
 *****************************************************************************/
#define DEBUG                       0                  
#define dbgprintf(...)              if (DEBUG) { printf(__VA_ARGS__); }
#define CRASH_TEST                  1
#define crash()                     if (CRASH_TEST) { *((char*)0) = 0; }
#define CACHE_ON                    1

#endif