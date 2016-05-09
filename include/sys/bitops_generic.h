#ifndef _SYS_BITOPS_GENERIC_H_
#define _SYS_BITOPS_GENERIC_H_

#define BITS_PER_LONG ((sizeof (unsigned long)) * 8)
#define BITS_PER_LONG_LONG ((sizeof (unsigned long long)) * 8)

#define GENMASK(h,l) \
        (((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#define GENMASK_ULL(h,l) \
        (((~0ULL) << (l)) & (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))

#endif
