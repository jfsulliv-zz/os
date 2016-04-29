#ifndef _SYS_PANIC_H_
#define _SYS_PANICH_H_

#define panic(why) do {                                 \
        _panic(__FILE__, __func__, __LINE__, why);      \
} while(0)

#define bug(why) do {                                   \
        _bug(__FILE__, __func__, __LINE__, why);        \
} while(0)

#define bug_on(cond,why) if (cond) {                    \
        _bug(__FILE__, __func__, __LINE__, why);        \
}

void _panic(const char *file, const char *fun, int line, const char *why);
void _bug(const char *file, const char *fun, int line, const char *why);

#endif
