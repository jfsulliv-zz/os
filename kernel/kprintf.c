#include <stdarg.h>
#include <stddef.h>
#include <sys/config.h>
#include <sys/kprintf.h>
#include <sys/stdio.h>
#include <sys/output_device.h>
#include <sys/string.h>

static output_device_t output_device = { NULL };


void
kprintf(kprintf_pri_t pri, const char *fmt, ...)
{
        va_list args;
        char tmp[4096];
        int ret;

        if (!output_device.puts)
                return;

        if (pri == PRI_DEBUG && !CONF_DEBUG)
                return;

        va_start (args, fmt);
        ret = vsnprintf(tmp, sizeof(tmp), fmt, args);
        if (ret > 0) {
                tmp[ret] = 0;
        } else {
                *tmp = 0;
        }
        output_device.puts(tmp);
}

void
kprintf_set_output_device(output_device_t *dev)
{
        output_device.puts = dev->puts;
}
