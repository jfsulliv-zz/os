#ifndef _SYS_SIZE_H_
#define _SYS_SIZE_H_

#define B_PER_KiB   1024
#define B_PER_KB    1000
#define KiB_PER_MiB 1024
#define KB_PER_MB   1000

#define B_PER_MiB   (B_PER_KiB * KiB_PER_MiB)

#define B_KiB(bytes) ((unsigned long)(bytes) / B_PER_KiB)
#define B_MiB(bytes) ((unsigned long)(bytes) / B_PER_MiB)

#endif
