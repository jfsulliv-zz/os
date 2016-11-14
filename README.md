# OS - Kernel Dev Project

This is my personal project to get better acquainted with kernel
development. The goal of this kernel is primarily personal development
(and fun!), but the kernel is developed to be a practical POSIX-like
environment, with a focus on modern development practices and tools.

So far, there is only support for x86 and x86_64, with ARM(64) as a
possible future target.

### What's done
- Page frame allocator
- Virtual memory allocator (SLAB allocator)
- System call entry/exit points (x86 only)
- Context switching (x86 only)
- Debugging support (stack traces, function name resolution, etc)

### In progress
- System calls and context switching (x86_64)
- Scheduler

### TODO
- Filesystem abstraction layer and some initial FS implementations
- Device abstraction layer, support for some hardware
- Network stack
- IPC
