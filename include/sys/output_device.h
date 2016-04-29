#ifndef _SYS_OUTPUT_DEVICE_H_
#define _SYS_OUTPUT_DEVICE_H_

typedef struct {
        void (*puts)(const char *str);
} output_device_t;

#endif
