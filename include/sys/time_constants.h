#ifndef _SYS_TIME_CONSTANTS_H_

/*
 * sys/time_constants.h - Conversion between time units
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 11/16
 */

#include <stdint.h>

#define MIN_PER_HR      (uint64_t)(60)

#define SEC_PER_MIN     (uint64_t)(60)
#define SEC_PER_HR      (uint64_t)(3600)

#define MSEC_PER_SEC    (uint64_t)(1000)
#define MSEC_PER_MIN    (uint64_t)(60000)
#define MSEC_PER_HR     (uint64_t)(3600000)

#define USEC_PER_MSEC   (uint64_t)(1000)
#define USEC_PER_SEC    (uint64_t)(1000000)
#define USEC_PER_MIN    (uint64_t)(60000000)
#define USEC_PER_HR     (uint64_t)(3600000000)

#define NSEC_PER_USEC   (uint64_t)(1000)
#define NSEC_PER_MSEC   (uint64_t)(1000000)
#define NSEC_PER_SEC    (uint64_t)(1000000000)
#define NSEC_PER_MIN    (uint64_t)(60000000000)
#define NSEC_PER_HR     (uint64_t)(3600000000000)

#endif
