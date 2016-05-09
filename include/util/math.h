#ifndef _UTIL_MATH_H_
#define _UTIL_MATH_H_

#define LOG2(X) \
        ((unsigned) (8*sizeof (unsigned long) - __builtin_clzl((X)) - 1))

static inline unsigned long next_pow2(unsigned long x)
{
        if (x != 1 && (x & (x-1)))
                return (LOG2(x) + 1);
        return LOG2(x);
}

#endif
