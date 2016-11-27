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

typedef struct process_state {
        struct regs uregs;              /* Saved user-mode registers */
        struct regs regs;               /* Saved kernel-mode registers */
        void *kstack;                   /* The kernel stack base */
        sched_state_t sched_state;      /* process execution state */
} proc_state_t;

#define PROC_STATE_INIT ((proc_state_t) \
{       .uregs = { 0 },                 \
        .regs = { 0 },                  \
        .kstack = NULL,                 \
        .sched_state = PROC_STATE_NEW,  \
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

/* The global process control block which contains:
 *  1) Process Identification data,
 *  2) Process State data,
 *  3) Process Control data
 * For a single process. */
typedef struct process {
        proc_id_t       id;             /* Identification */
        proc_state_t    state;          /* Execution state */
        proc_control_t  control;        /* Process contol data */
} proc_t;

#define PROC_INIT(p) ((proc_t)\
{       .id = PROC_ID_INIT,             \
        .state = PROC_STATE_INIT,       \
        .control = PROC_CONTROL_INIT(p.control), \
})

/* The process table, indexed by PIDs */
extern proc_t **proc_table;
extern pid_t    pid_max;

/* The init process */
extern proc_t *init_procp;

/* Initialize the proc subsystem. */
void proc_system_init(void);
/* Early initialization, before we have VMA. */
void proc_system_early_init(void);
__test void proc_test(void);

/* Returns the current process who is executing. */
proc_t *current_process(void);

/* Sets the maximum number of PIDs the system can have. Only supports
 * increasing the pidmax. Returns 1 on failure (i.e. if out of memory)
 */
int set_pidmax(pid_t);

/* A memory cache for handing out proc_t structs */
extern mem_cache_t *proc_alloc_cache;

/* Returns the PCB for the given pid. */
proc_t *find_process(pid_t);

/* A set of flags for a fork request. */
typedef struct {
        int             fr_flags;
} fork_req_t;

/* Copy the given process with the given fork_req_t flags. */
proc_t *copy_process(proc_t *, fork_req_t *);

/* Free the resources held by the given process. */
void free_process(proc_t *);

/* Switch the running process to nextp.
 * We save the register state into the PCB of current() at this point. */
void switch_process(proc_t *nextp);

/* Initializers and deinitializers for PCBs. */
void proc_init(proc_t *);
void proc_deinit(proc_t *);

#endif
