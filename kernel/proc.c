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

#include <mm/pmm.h>
#include <mm/vma.h>
#include <sys/string.h>
#include <sys/panic.h>
#include <sys/proc.h>

proc_t init_proc;

proc_t **proc_table;
pid_t    pid_max = 65535; /* Default value */

mem_cache_t *proc_alloc_cache;

void proc_ctor(void *p, __attribute__((unused)) size_t sz)
{
        proc_t *proc = (proc_t *)p;
        if (!p)
                return;
        proc_init(proc);
}

void proc_dtor(void *p, __attribute__((unused)) size_t sz)
{
        proc_t *proc = (proc_t *)p;
        if (!p)
                return;
        proc_deinit(proc);
}

int
set_pidmax(pid_t p)
{
        proc_t **tmp;
        if (p < pid_max)
                return 1; /* Don't let the table shrink */
        tmp = krealloc(proc_table, (1 + p) * sizeof(void *), M_KERNEL);
        if (!tmp)
                return 1;
        memset(proc_table + pid_max + 1, 0, (1 + p - pid_max)
                                            * sizeof(void *));
        proc_table = tmp;
        pid_max    = p;
        return 0;
}

static void
proc_system_init_table(void)
{
        proc_table = kmalloc((1 + pid_max) * sizeof(void *), M_KERNEL);
        bug_on(!proc_table, "Failed to allocate process table.");
        memset(proc_table, 0, (1 + pid_max) * (sizeof(void *)));
}

static void
proc_system_init_initproc(void)
{
        init_proc = PROC_INIT(init_proc);
        init_proc.id.pid = 1;
        init_proc.id.ppid = 0;
        init_proc.control.pmm = &init_pmm;
        proc_table[1] = &init_proc;
}

static void
proc_system_init_alloc(void)
{
        proc_alloc_cache = mem_cache_create("pcb_cache", sizeof(proc_t),
                                             sizeof(proc_t),
                                             0, proc_ctor, proc_dtor);
        bug_on(!proc_alloc_cache, "Failed to create pcb_cache");
}

void
proc_system_init(void)
{
        proc_system_init_alloc();
        proc_system_init_table();
        proc_system_init_initproc();
}

static proc_t *
_alloc_process(void)
{
        proc_t *p;
        p = mem_cache_alloc(proc_alloc_cache, M_KERNEL);
        if (p) {
                /* Check for errors */
                if (p->id.pid == 0) /* Out of PIDs */
                        goto free_proc;
                else if (p->control.pmm == NULL) /* Out of memory */
                        goto free_proc;
        }
        return p;
free_proc:
        mem_cache_free(proc_alloc_cache, p);
        return NULL;
}

proc_t *
find_process(pid_t pid)
{
        if (pid <= 0 || pid > pid_max)
                return NULL;
        return proc_table[pid];
}

static int
_make_child(proc_t *par, proc_t *p)
{
        p->id.ppid = par->id.pid;
        list_add(&par->control.children, &p->control.pr_list);
        return pmm_copy(p->control.pmm, par->control.pmm);
}

proc_t *
copy_process(proc_t *par, fork_req_t *req)
{
        if (!par)
                return NULL;

        proc_t *p = _alloc_process();
        if (!p)
                return NULL;
        if (_make_child(par, p)) {
                free_process(p);
                return NULL;
        }
        return p;
}

void
free_process(proc_t *p)
{
        if (!p)
                return;
        bug_on(p->state.sched_state != PROC_STATE_TERMINATED,
               "Process was freed while in use.");
        mem_cache_free(proc_alloc_cache, p);
}

static pid_t
next_pid(void)
{
        static pid_t last_pid = 1;
        pid_t ret;
        for (ret = last_pid+1; ret != last_pid+1; ret++)
        {
                if (ret > pid_max)
                        ret = 1;
                if (proc_table[ret] == NULL) {
                        last_pid = ret;
                        return ret;
                }
        }
        return 0; /* Out of PIDs */
}

void
proc_init(proc_t *p)
{
        if (!p)
                return;
        p->id      = PROC_ID_INIT;
        p->id.pid  = next_pid();
        p->state   = PROC_STATE_INIT;
        p->control = PROC_CONTROL_INIT(p->control);
        p->control.pmm = pmm_create();
}

void
proc_deinit(proc_t *p)
{
        if (!p)
                return;
        proc_table[p->id.pid] = NULL;
}
