#ifndef _MM_PAGE_TABLE_H_
#define _MM_PAGE_TABLE_H_

#include <mm/flags.h>
#include <sys/bitops_generic.h>

typedef unsigned int pgflags_t;

#define _PAGE_BIT_PRESENT       0
#define _PAGE_BIT_RW            1
#define _PAGE_BIT_USER          2
#define _PAGE_BIT_PWT           3
#define _PAGE_BIT_PCD           4
#define _PAGE_BIT_ACCESSED      5
#define _PAGE_BIT_DIRTY         6

#define _PAGE_PRESENT   0x001
#define _PAGE_RW        0x002
#define _PAGE_USER      0x004
#define _PAGE_PWT       0x008
#define _PAGE_PCD       0x010
#define _PAGE_ACCESSED  0x020
#define _PAGE_DIRTY     0x040

#define _PAGE_PROTNONE  0x080   /* If not present */

#define PAGE_FLAGS_MASK (GENMASK(11, 0))
#define PAGE_ADDR_MASK  (~PAGE_FLAGS_MASK)

#define PAGE_TAB  (_PAGE_PRESENT | _PAGE_USER | _PAGE_RW | \
                   _PAGE_ACCESSED | _PAGE_DIRTY)
#define KPAGE_TAB (_PAGE_PRESENT | _PAGE_RW | _PAGE_ACCESSED | \
                   _PAGE_DIRTY)

static inline pgflags_t
mflags_to_pgflags(mflags_t fl)
{
        pgflags_t ret = _PAGE_PRESENT | _PAGE_RW;
        if (fl & M_HIGH)
                ret |= _PAGE_USER;
        return ret;
}

#endif
