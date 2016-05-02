#ifndef _MM_FLAGS_H_
#define _MM_FLAGS_H_

#include <stdint.h>

#define M_WAIT_BIT 0
#define M_HIGH_BIT 1
#define M_DMA_BIT  2

#define M_WAIT     (1 << M_WAIT_BIT)
#define M_HIGH     (1 << M_HIGH_BIT)
#define M_DMA      (1 << M_DMA_BIT)

#define M_BUFFER   (         M_WAIT)
#define M_ATOMIC   (0)
#define M_USER     (M_HIGH | M_WAIT)
#define M_KERNEL   (         M_WAIT)

typedef uint32_t mflags_t;

#endif
