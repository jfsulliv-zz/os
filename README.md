# OS - Kernel Dev Project

This is my personal project to get better acquainted with kernel
development. The goal of this kernel is primarily personal development
(and fun!), but the kernel is developed to be a practical POSIX-like
environment, with a focus on modern development practices and tools.

So far, there is only support for x86 and x86_64, with ARM(64) as a
possible future target.

### Features
- Support for x86 (i386) and x86_64 (i686) platforms
- Virtual memory allocator (slab)
- Debugging support (stack tracing, function name resolution, etc)
- Very basic process support

### In progress
- System calls and context switching

### TODO
- Filesystem abstraction layer and some initial FS implementations
- Device abstraction layer, support for some hardware
- Network stack
- Process model, synchronization primitives
- IPC
- Scheduler
