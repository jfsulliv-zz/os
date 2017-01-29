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

#include <mm/vma.h>
#include <mm/vmobject.h>
#include <mm/paging.h>
#include <sys/panic.h>
#include <sys/sysinit.h>

static mem_cache_t *vmobject_cache;

static void vmobject_ctor(void *p, __attribute__((unused))size_t sz)
{
        vmobject_t *obj = (vmobject_t *)p;

        obj->refct = 0;
        obj->size = 0;
        obj->page = NULL;
}

vmobject_t *
vmobject_create_anon(size_t sz, pflags_t flags)
{
        if (sz == 0 || BAD_PFLAGS(flags))
                return NULL;
        vmobject_t *obj = mem_cache_alloc(vmobject_cache, M_KERNEL);
        if (!obj)
                return NULL;
        obj->size = PAGE_ROUNDUP(sz);
        obj->pflags = flags;
        return obj;

}

void
vmobject_destroy(vmobject_t *object)
{
        bug_on(!object, "Destoying NULL object");
        bug_on(object->refct > 0, "Destroying referenced object");
}

static int
vmobject_init(void)
{
        vmobject_cache =
                mem_cache_create("vmobject_cache", sizeof(vmobject_t),
                                 sizeof(vmobject_t), 0,
                                 vmobject_ctor, NULL);

        bug_on(!vmobject_cache, "Failed to allocate vmobject cache");
        return 0;
}
SYSINIT_STEP("vmobject", vmobject_init, SYSINIT_VMOBJ, 0);
