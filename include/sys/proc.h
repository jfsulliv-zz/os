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
#include <mm/vma.h>
#include <mm/vmman.h>
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
        struct regs regs;               /* CPU's registers */
        void *sp;                       /* stack pointer */
        void *bp;                       /* frame pointer */
        void *cp;                       /* code pointer */
        sched_state_t sched_state;      /* process execution state */
} proc_state_t;

#define PROC_STATE_INIT ((proc_state_t)\
{       .regs = { 0 },                  \
        .sp   = NULL,                   \
        .bp   = NULL,                   \
        .cp   = NULL,                   \
        .sched_state = PROC_STATE_NEW,  \
})

typedef struct process_control {
        struct list_head children;      /* Child process list */
        struct list_head pr_list;       /* Entry in the above list */
        vmman_t vm;                     /* Virtual Memory info */
} proc_control_t;

#define PROC_CONTROL_INIT(c) ((proc_control_t)\
{       .children = LIST_HEAD_INIT(c.children), \
        .pr_list = LIST_HEAD_INIT(c.pr_list),   \
        .vm = { 0 }                             \
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

extern proc_t init_proc; /* The init process */

/* The process table, indexed by PIDs */
extern proc_t **proc_table;
extern pid_t    pid_max;

#ifdef CONF_VMA_SLAB
extern slab_cache_t *proc_alloc_cache;
#endif

int
set_pidmax(pid_t);

void
proc_system_init(void);

proc_t *
find_process(pid_t);

proc_t *
make_child_process(proc_t *);

void
free_process(proc_t *);

void
proc_init(proc_t *);

void
proc_deinit(proc_t *);

#endif
