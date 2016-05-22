#ifndef _MM_PROTFLAGS_H_
#define _MM_PROTFLAGS_H_

#include <stdint.h>
#include <sys/bitops_generic.h>

#define PFLAGS_READ_BIT         0
#define PFLAGS_WRITE_BIT        1
#define PFLAGS_EXEC_BIT         2

#define PFLAGS_GOOD_MASK        (GENMASK(3, 0))
#define BAD_PFLAGS(f)           ((f) & ~PFLAGS_GOOD_MASK)

#define PFLAGS_READ             (1 << PFLAGS_READ_BIT)
#define PFLAGS_WRITE            (1 << PFLAGS_WRITE_BIT)
#define PFLAGS_EXEC             (1 << PFLAGS_EXEC_BIT)

#define PFLAGS_R                (PFLAGS_READ                             )
#define PFLAGS_W                (              PFLAGS_WRITE              )
#define PFLAGS_X                (                             PFLAGS_EXEC)
#define PFLAGS_RW               (PFLAGS_READ | PFLAGS_WRITE              )
#define PFLAGS_RX               (PFLAGS_READ                | PFLAGS_EXEC)
#define PFLAGS_WX               (              PFLAGS_WRITE | PFLAGS_EXEC)
#define PFLAGS_RWX              (PFLAGS_READ | PFLAGS_WRITE | PFLAGS_EXEC)

typedef uint32_t pflags_t;

#endif
