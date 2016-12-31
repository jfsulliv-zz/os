/*
Copyright (c) 2016, James Sullivan <sullivan.james.f@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote
      products derived from this software without specific prior
      written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER>
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

/*
 * sys/proc.h - Process Control Block
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 05/16
 */

#include <machine/percpu.h>
#include <machine/regs.h>
#include <mm/pmm.h>
#include <mm/vma.h>
#include <sched/schedinfo.h>
#include <sys/debug.h>
#include <util/list.h>

typedef int32_t  pid_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;

/* Information which is used to identify a process. */
typedef struct process_id {
        pid_t pid;                      /* PID */
        pid_t ppid;                     /* parent's PID */
        pid_t sid;                      /* session ID */
        uid_t uid;                      /* real user ID */
        gid_t gid;                      /* real group ID */
        uid_t euid;                     /* effective user ID */
        gid_t egid;                     /* effective group ID */
        uid_t suid;                     /* saved set-user ID */
        gid_t sgid;                     /* saved set-group ID */
} proc_id_t;

#define PROC_ID_INIT ((proc_id_t)\
{       .pid = 0,                       \
        .ppid = 0,                      \
        .sid = 0,                       \
        .uid = 0,                       \
        .gid = 0,                       \
        .euid = 0,                      \
        .egid = 0,                      \
        .suid = 0,                      \
        .sgid = 0,                      \
})

typedef enum {
        PROC_STATE_NEW,                 /* Freshly forked */
        PROC_STATE_READY,               /* Ready for scheduling */
        PROC_STATE_WAITING,             /* Sitting in the sched queue */
        PROC_STATE_RUNNING,             /* Running */
        PROC_STATE_BLOCKED,             /* Blocking */
        PROC_STATE_TERMINATED,          /* Dead */
        PROC_STATE_SUSP_WAITING,        /* Swapped out and waiting */
        PROC_STATE_SUSP_BLOCKED,        /* Swapped out and blocked */
        PROC_STATE_INVAL = -1,
} sched_state_t;

typedef enum {
        PROC_CONTEXT_USER,              /* Running in userspace */
        PROC_CONTEXT_KERN,              /* Running in kernel */
        PROC_CONTEXT_INTR,              /* Running in an intrrupt */
        PROC_CONTEXT_NONE,              /* Not running */
        PROC_CONTEXT_INVAL = -1,
} sched_context_t;

typedef struct process_state {
        struct regs uregs;              /* Saved user-mode registers */
        struct regs regs;               /* Saved kernel-mode registers */
        void *kstack;                   /* The kernel stack base */
        sched_state_t sched_state;      /* process execution state */
        sched_context_t sched_context;  /* process execution context */
} proc_state_t;

#define PROC_STATE_INIT ((proc_state_t) \
{       .uregs = { 0 },                 \
        .regs = { 0 },                  \
        .kstack = NULL,                 \
        .sched_state = PROC_STATE_NEW,  \
        .sched_context = PROC_CONTEXT_NONE,  \
})

struct process;

typedef struct process_control {
        struct list_head children;      /* Child process list */
        struct list_head pr_list;       /* Entry in the above list */
        struct sched_info schedinfo;    /* Scheduler metadata */
        pmm_t *pmm;                     /* Physical memory mappings */
} proc_control_t;

#define PROC_CONTROL_INIT(c) ((proc_control_t)  \
{       .children = LIST_HEAD_INIT(c.children), \
        .pr_list = LIST_HEAD_INIT(c.pr_list),   \
        .schedinfo = { 0 },                     \
        .pmm = NULL,                            \
})

typedef struct process_resources {
        uint64_t rtime_us;              /* Real time executing */
        unsigned long timeslice_start_us;    /* When the timeslice started */
        uint64_t u_ticks;               /* Sched ticks in userspace */
        uint64_t k_ticks;               /* kernelspace */
        uint64_t i_ticks;               /* interrupt context */
        uint64_t all_ticks;             /* sum of all ticks */
} proc_res_t;

/* The global process control block which contains:
 *  1) Process Identification data,
 *  2) Process State data,
 *  3) Process Control data
 * For a single process. */
typedef struct process {
        proc_id_t       id;             /* Identification */
        proc_state_t    state;          /* Execution state */
        proc_control_t  control;        /* Process contol data */
        proc_res_t      resource;       /* Resource counters */
} proc_t;

#define PROC_INIT(p) ((proc_t)\
{       .id = PROC_ID_INIT,             \
        .state = PROC_STATE_INIT,       \
        .control = PROC_CONTROL_INIT(p.control), \
})

/* The process table, indexed by PIDs */
extern proc_t **proc_table;
extern pid_t    pid_max;

/* The idle process */
extern proc_t *idle_procp;

/* The init process */
extern proc_t *init_procp;

/* Returns the current process who is executing. */
#define proc_current()  PERCPU_CURPROC

/* Sets the current process which is executing. */
void proc_set_current(proc_t *);

/* Initialize the proc subsystem. */
void proc_system_init(void);
/* Early initialization, before we have VMA. */
void proc_system_early_init(void);
__test void proc_test(void);

/* Sets the maximum number of PIDs the system can have. Only supports
 * increasing the pidmax. Returns 1 on failure (i.e. if out of memory)
 */
int set_pidmax(pid_t);

/* A memory cache for handing out proc_t structs */
extern mem_cache_t *proc_alloc_cache;

/* Returns the PCB for the given pid. */
proc_t *find_process(pid_t);

/* A set of flags for a fork request. */
typedef int fork_req_t;

#define FORK_FLAGS_COPYUSER     0b1

/* Copy the given process with the given fork_req_t flags. */
proc_t *copy_process(proc_t *, fork_req_t);

/* Free the resources held by the given process. */
void free_process(proc_t *);

/* Initializers and deinitializers for PCBs. */
void proc_init(proc_t *);
void proc_deinit(proc_t *);

#endif
