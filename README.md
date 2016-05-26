# OS - Kernel Dev Project
## (With no catchy name)

This is my personal project to get better acquainted with kernel
development. There are no particular goals or deadlines for this
project, and I work on it when the mood strikes.

So far, there is only support for x86 and x86_64, with ARM(64) as a
possible future target.

### Features
- Support for x86 (i386) and x86_64 (i686) platforms
- Virtual memory allocator (slab)
- Debugging support (stack tracing, function name resolution, etc)

### TODO
- Filesystem abstraction layer and some initial FS implementations
- Device abstraction layer, support for some hardware
- Network stack
- Process model, synchronization primitives
- IPC
- Scheduler
- Making a useful userspace (syscalls, pid 1, etc...)
