#ifndef __QN_OS_LINUX_TYPES_H__
#define __QN_OS_LINUX_TYPES_H__

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef unsigned int qn_uint;
typedef uint16_t qn_uint16;
typedef uint32_t qn_uint32;
typedef uint64_t qn_uint64;
typedef ssize_t qn_ssize;
typedef size_t qn_size;

typedef bool qn_bool;

static const qn_bool qn_false = false;
static const qn_bool qn_true = true;

#if defined(QN_CFG_SMALL_NUMBERS)

typedef int qn_integer;         // 4 Bytes for ILP32 and LP64
typedef float qn_number;        // 4 Bytes for ILP32 and LP64

#else

typedef long long qn_integer;   // 8 Bytes for ILP32 and LP64
typedef double qn_number;       // 8 Bytes for ILP32 and LP64

#endif

#if defined(QN_CFG_LARGE_FILE_SUPPORT)

typedef long long qn_fsize; // A signed long long integer for file sizes.
typedef off64_t qn_foffset; // A signed long long integer for file offsets.

#else

typedef long qn_fsize;      // A signed long integer for file sizes.
typedef off_t qn_foffset;   // A signed long integer for file offsets.

#endif

typedef time_t qn_time;

#endif // __QN_OS_LINUX_TYPES_H__

