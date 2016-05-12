#ifndef _SYS_KPRINTF_H_
#define _SYS_KPRINTF_H_

#include <stdarg.h>
#include <sys/output_device.h>

typedef enum {
        PRI_ALL = 0,
        PRI_DEBUG,
        PRI_ERR,
        INV = -1,
} kprintf_pri_t;

void kprintf(kprintf_pri_t pri, const char *fmt, ...);
void kprintf_set_output_device(output_device_t *dev);

#define debug(msg, ...) kprintf(PRI_DEBUG, msg, ##__VA_ARGS__)

#endif
