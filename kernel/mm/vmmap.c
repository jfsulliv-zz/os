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

#include <mm/vmmap.h>

#include <mm/vma.h>
#include <mm/vmobject.h>
#include <mm/paging.h>
#include <mm/pfa.h>
#include <sys/debug.h>
#include <sys/errno.h>
#include <sys/panic.h>
#include <sys/proc.h>
#include <sys/sysinit.h>
#include <util/cmp.h>

static mem_cache_t *vmmap_area_cache;

// Creates a new mapping. 'object' may be null.
static vmmap_area_t *
vmmap_area_create(vaddr_t start, unsigned long size,
                  vmobject_t *object, unsigned long offset);

static void
vmmap_area_destroy(vmmap_area_t *);

static bool
vmmap_area_mapped(vmmap_t *map, vaddr_t addr, unsigned long size);

static int
vmmap_area_link(vmmap_t *map, vmmap_area_t *area);

static void
vmmap_area_unlink(vmmap_t *map, vmmap_area_t *area);

static vmmap_area_t *
vmmap_find_successor(vmmap_area_t *area);

static vmmap_area_t *
vmmap_find_predecessor(vmmap_area_t *area);

void
vmmap_init(vmmap_t *map, pmm_t *pmm)
{
        bug_on(!map, "NULL map initialized");
        map->pmm = pmm;
        map->avl_head = map->areap = NULL;
}

void
vmmap_deinit(vmmap_t *map)
{
        bug_on(!map, "NULL map initialized");
        vmmap_area_t *p, *n;
        p = map->areap;
        while (p)
        {
                n = p->next;
                vmmap_area_destroy(p);
                p = n;
        }
}

int vmmap_map_object_at(vmmap_t *map, vmobject_t *object,
                        unsigned long offset, vaddr_t start,
                        unsigned long size)
{
        bug_on(!map, "NULL map to insert object into");
        bug_on(!object, "NULL vmobject being inserted");

        offset = PAGE_ROUND(offset);
        start = PAGE_ROUND(start);
        size = PAGE_ROUNDUP(size);

        bug_on(offset + size > object->size,
                "Map would exceed object limits");

        if (vmmap_area_mapped(map, start, size))
                return EAGAIN;

        vmmap_area_t *area = vmmap_area_create(start, size,
                                               object, offset);
        if (!area)
                return ENOMEM;
        bug_on(vmmap_area_link(map, area), "Linking overlapped region");
        return 0;
}

vmmap_area_t *
vmmap_find(const vmmap_t *map, vaddr_t addr)
{
        vmmap_area_t *area = map->avl_head;
        while (area)
        {
                if (area->start <= addr) {
                        if (area->start + area->size > addr)
                                break;
                        area = area->right;
                } else if (area->start > addr) {
                        area = area->left;
                } else {
                        break;
                }
        }
        return area;
}

static bool
vmmap_area_mapped(vmmap_t *map, vaddr_t addr, unsigned long size)
{
        vmmap_area_t *area = map->avl_head;
        while (area)
        {
                if (area->start <= addr) {
                        if (area->start + area->size > addr)
                                return true;
                        area = area->right;
                } else if (area->start > addr) {
                        if (area->start < addr + size)
                                return true;
                        area = area->left;
                } else {
                        return true;
                }
        }
        return false;
}

/* Unmaps [addr, addr+size) from 'area' (which must contain this
 * region). If the entire region is removed, also deletes 'area'.
 * Returns the number of bytes removed. */
static size_t
vmmap_partial_unmap(vmmap_t *map, vmmap_area_t *area, vaddr_t addr,
                    unsigned long size)
{
        size_t removed;
        if (area->start == addr) {
                if (area->size == size) {
                        // Full region should be unmapped
                        vmmap_area_unlink(map, area);
                        vmmap_area_destroy(area);
                        removed = size;
                } else {
                        // End of the region should be unmapped
                        // (just truncate the end)
                        area->size = size;
                        removed = size - area->size;
                }
        } else {
                if (area->start + area->size == addr + size) {
                        // Start of the region should be unmapped
                        // (just truncate the start)
                        area->start = addr;
                        removed = addr - area->start;
                } else {
                        // area needs to be split. TODO
                        bug("TODO");
                }
        }
        size_t npg = size / PAGE_SIZE;
        pmm_unmap_range(map->pmm, addr, npg, NULL);
        return removed;
}

void
vmmap_remove(vmmap_t *map, vaddr_t addr, unsigned long size)
{
        addr = PAGE_ROUND(addr);
        size = PAGE_ROUNDUP(size);
        while (size > 0)
        {
                vmmap_area_t *area = vmmap_find(map, addr);
                size_t removed =
                        vmmap_partial_unmap(map, area, addr, size);
                addr += removed;
                size -= removed;
        }
}

/* Insert 'area' into 'map' if possible. Breaks AVL invariants. 
 * Returns 1 if the space is occupied. */
static int
vmmap_area_link_simple(vmmap_t *map, vmmap_area_t *area)
{
        vmmap_area_t *ins = map->avl_head;
        if (!ins) {
                map->avl_head = area;
                return 0;
        }
        vmmap_area_t *prev = NULL;
        while (ins)
        {
                prev = ins;
                if (ins->start >= area->start + area->size) {
                        ins = ins->left;
                } else if (ins->start + ins->size <= area->start) {
                        ins = ins->right;
                } else {
                        return 1;
                }
        }
        if (area->start < prev->start) {
                prev->left = area;
        } else {
                prev->right = area;
        }
        area->par = prev;
        return 0;
}

static inline unsigned int
vmmap_avl_height(vmmap_area_t *area)
{
        return area ? area->height : 0;
}

static inline unsigned int
vmmap_avl_children_max_height(vmmap_area_t *area)
{
        return MAX(vmmap_avl_height(area->left),
                   vmmap_avl_height(area->right));
}

static inline vmmap_area_t *
vmmap_avl_rotate_left(vmmap_area_t *area)
{
        vmmap_area_t *root = area->right;
        vmmap_area_t *t = root->left;

        root->par = area->par;
        area->par = root;
        root->left = area;

        area->right = t;
        if (t) {
                t->height = vmmap_avl_children_max_height(t) + 1;
                t->par = area;
        }
        area->height = vmmap_avl_children_max_height(area) + 1;
        root->height = vmmap_avl_children_max_height(root) + 1;

        return root;
}

static inline vmmap_area_t *
vmmap_avl_rotate_right(vmmap_area_t *area)
{
        vmmap_area_t *root = area->left;
        vmmap_area_t *t = root->right;

        root->par = area->par;
        area->par = root;
        root->right = area;

        area->left = t;
        if (t) {
                t->height = vmmap_avl_children_max_height(t) + 1;
                t->par = area;
        }
        area->height = vmmap_avl_children_max_height(area) + 1;
        root->height = vmmap_avl_children_max_height(root) + 1;

        return root;
}

static inline int
vmmap_avl_balance(vmmap_area_t *area)
{
        return vmmap_avl_height(area->right) -
                vmmap_avl_height(area->left);
}

/* Rebalances the tree at 'u'. Assumes that the balance of u is skewed
 * by +/- 2 or more (otherwise we could deref null pointers).
 * Returns the node replacing 'u'. */
static vmmap_area_t *
vmmap_area_rebalance(vmmap_area_t *u)
{
        vmmap_area_t *larger_c = vmmap_avl_balance(u) < 0
                ? u->left
                : u->right;
        vmmap_area_t *larger_gc = vmmap_avl_balance(larger_c) < 0
                ? larger_c->left
                : larger_c->right;

        /* Perform one of four swaps to re-balance the tree.
         * 'r' will contain the replacement for 'u'. */
        vmmap_area_t *r;
        if (larger_c == u->left) {
                if (larger_gc == larger_c->left) {
                        r = vmmap_avl_rotate_right(u);
                } else {
                        u->left = vmmap_avl_rotate_left(larger_c);
                        r = vmmap_avl_rotate_right(u);
                }
        } else {
                if (larger_gc == larger_c->left) {
                        u->right = vmmap_avl_rotate_right(larger_c);
                        r = vmmap_avl_rotate_left(u);
                } else {
                        r = vmmap_avl_rotate_left(u);
                }
        }
        return r;
}

/* Links 'area' into the linked-list of the tree that it is a part of. */
static void
vmmap_area_link_list(vmmap_area_t *area)
{
        vmmap_area_t *succ, *pred;
        succ = vmmap_find_successor(area);
        if (succ) {
                pred = succ->prev;
        } else {
                pred = vmmap_find_predecessor(area);
        }

        if (pred) {
                pred->next = area;
                area->prev = pred;
        }
        if (succ) {
                area->next = succ;
                succ->prev = area;
        }
}

/* Links 'area' into 'map'. Assumes both are initialized.
 * Returns 0 on success and 1 if the area would overlap another. */
static int
vmmap_area_link(vmmap_t *map, vmmap_area_t *area)
{
        int ret = vmmap_area_link_simple(map, area);
        if (ret) {
                return ret;
        }

        /* Seek up for the unbalanced node */
        vmmap_area_t *u = area;
        while (u)
        {
                u->height = vmmap_avl_children_max_height(u) + 1;
                int balance = vmmap_avl_balance(u);
                if (balance < -1 || balance > 1) {
                        break;
                }
                u = u->par;
        }
        if (u) {
                vmmap_area_t *p = u->par;
                vmmap_area_t *r = vmmap_area_rebalance(u);
                if (p) {
                        if (p->left == u)
                                p->left = r;
                        else
                                p->right = r;
                } else {
                        map->avl_head = r;
                }
        }
        vmmap_area_link_list(area);
        if (!area->prev) {
                // Set the start of the linked-list if there is no
                // earlier node
                map->areap = area;
        }
        return 0;
}

static vmmap_area_t *
vmmap_avl_min(vmmap_area_t *area)
{
        while (area->left)
                area = area->left;
        return area;
}

static vmmap_area_t *
vmmap_avl_max(vmmap_area_t *area)
{
        while (area->right)
                area = area->right;
        return area;
}

/* Unlinks 'area' from the AVL tree, when 'area' has two children.
 * Returns the node that replaced 'area'. */
static vmmap_area_t *
vmmap_area_unlink_simple_deep(vmmap_t *map, vmmap_area_t *area)
{
        vmmap_area_t *p = area->par;
        vmmap_area_t *r = area->right;
        vmmap_area_t *s = vmmap_avl_min(r);

        // Swap 's' into 'area', saving its right subtree and parent for
        // later
        vmmap_area_t *s_right = s->right;
        vmmap_area_t *s_par = s->par;
        s->left = area->left;
        s->right = area->right;
        s->par = p;
        s->height = area->height;
        if (p) {
                if (area == p->left)
                        p->left = s;
                else
                        p->right = s;
        } else {
                map->avl_head = s;
        }

        // Replace the old position of 's' with its right subtree
        if (s_par->right == s) {
                s_par->right = s_right;
        } else {
                s_par->left = s_right;
        }

        // Update the heights of the modified tree by tracking up to
        // 's'. The caller will update the height from 's' to the top.
        vmmap_area_t *h = s_right->par;
        while (h != s)
        {
                h->height = vmmap_avl_children_max_height(h) + 1;
                h = h->par;
        }
        return s;
}

/* Unlinks 'area' from the AVL tree, breaking its invariants.
 * Returns the node that replaced 'area'. */
static vmmap_area_t *
vmmap_area_unlink_simple(vmmap_t *map, vmmap_area_t *area)
{
        vmmap_area_t *r;
        vmmap_area_t *p = area->par;
        if (!area->left || !area->right) {
                r = area->left ?: area->right;
                if (area->par) {
                        vmmap_area_t *p = area->par;
                        if (p->left == area)
                                p->left = r;
                        else
                                p->right = r;
                } else {
                        map->avl_head = r;
                }
                if (r) {
                        r->par = area->par;
                }
        } else {
                r = vmmap_area_unlink_simple_deep(map, area);
        }
        // Adjust the heights from the old parent of 'area' upwards
        vmmap_area_t *h = p;
        while (h)
        {
                h->height = vmmap_avl_children_max_height(h) + 1;
                h = h->par;
        }
        return r;
}

static void
vmmap_area_unlink(vmmap_t *map, vmmap_area_t *area)
{
        vmmap_area_t *p = area->par;
        vmmap_area_t *r = vmmap_area_unlink_simple(map, area);
        vmmap_area_t *u = r ?: p;

        if (!p) {
                map->avl_head = r;
        }

        // Seek upwards for unbalanced nodes (there may be several).
        while (u)
        {
                u->height = vmmap_avl_children_max_height(u) + 1;
                int balance = vmmap_avl_balance(u);
                if (balance < -1 || balance > 1) {
                        vmmap_area_t *p = u->par;
                        vmmap_area_t *r = vmmap_area_rebalance(u);
                        if (p) {
                                if (p->left == u)
                                        p->left = r;
                                else
                                        p->right = r;
                        } else {
                                map->avl_head = r;
                        }
                        u = r;
                }
                u = u->par;
        }
        if (area == map->areap) {
                map->areap = area->next;
        }
}

static vmmap_area_t *
vmmap_find_successor(vmmap_area_t *area)
{
        if (area->right) {
                return vmmap_avl_min(area->right);
        }
        vmmap_area_t *p = area;
        while (p->par)
        {
                if (p->par->left == p)
                        break;
                else
                        p = p->par;
        }
        return p->par;
}

static vmmap_area_t *
vmmap_find_predecessor(vmmap_area_t *area)
{
        if (area->left) {
                return vmmap_avl_max(area->left);
        }
        vmmap_area_t *p = area;
        while (p->par)
        {
                if (p->par->right == p)
                        break;
                else
                        p = p->par;
        }
        return p->par;
}


static vmmap_area_t *
vmmap_area_create(vaddr_t start, unsigned long size,
                  vmobject_t *object, unsigned long offset)
{
        vmmap_area_t *area = mem_cache_alloc(vmmap_area_cache,
                                             M_KERNEL);
        if (!area)
                return area;

        area->start = start;
        area->size = size;
        area->object = object;
        area->offset = offset;
        // TODO lock(object)
        object->refct++;
        // TODO unlock(object)
        return area;
}

static void
vmmap_area_destroy(vmmap_area_t *area)
{
        mem_cache_free(vmmap_area_cache, area);
}

static void
vmmap_area_ctor(void *p, __attribute__((unused))size_t sz)
{
        vmmap_area_t *vmmap_area = (vmmap_area_t *)p;
        vmmap_area->object = NULL;
        vmmap_area->next = vmmap_area->prev = NULL;
        vmmap_area->left = vmmap_area->right = vmmap_area->par = NULL;
        vmmap_area->height = 1;
}

static void
vmmap_area_dtor(void *p, __attribute__((unused))size_t sz)
{
        vmmap_area_t *vmmap_area = (vmmap_area_t *)p;

        if (vmmap_area->object) {
                bug_on(vmmap_area->object->refct == 0,
                       "Object refcount dropping below 0");
                // TODO lock(object)
                if (--vmmap_area->object->refct == 0) {
                        vmobject_destroy(vmmap_area->object);
                        vmmap_area->object = NULL;
                } else {
                        // TODO unlock(object)
                }
        }
        if (vmmap_area->prev) {
                vmmap_area->prev->next = vmmap_area->next;
        }
        if (vmmap_area->next) {
                vmmap_area->next->prev = vmmap_area->prev;
        }
}

static void __test
vmmap_test_insert(vmmap_t *map, vmobject_t *obj)
{
        vmmap_init(map, proc_current()->control.pmm);

        bug_on(vmmap_map_object_at(map, obj, 0, 0x40000, PAGE_SIZE),
                "Mapping failed");
        bug_on(obj->refct != 2, "Refct not bumped");
        bug_on(!map->avl_head ||
                map->avl_head->start != 0x40000, "AVL tree not started");
        bug_on(!map->areap ||
                map->areap->start != 0x40000, "Linked list not started");

        vmmap_deinit(map);
}

static void __test
vmmap_test_insert_left(vmmap_t *map, vmobject_t *obj)
{
        vmmap_init(map, proc_current()->control.pmm);

        vmmap_map_object_at(map, obj, 0, 0x40000, PAGE_SIZE);

        bug_on(vmmap_map_object_at(map, obj, 0, 0x20000, PAGE_SIZE),
                        "Mapping failed");
        bug_on(obj->refct != 3, "Refct not bumped");

        // AVL properties
        bug_on(map->avl_head->start != 0x40000,
                "AVL tree head not updated");
        bug_on(map->avl_head->left->start != 0x20000,
                "Left subtree not set");
        bug_on(map->avl_head->right,
                "Right subtree was set");
        bug_on(map->avl_head->left->par->start != 0x40000,
                "Left subtree parent pointer not set");
        bug_on(map->avl_head->left->left || map->avl_head->left->right,
                "Left subtree has children");
        bug_on(map->avl_head->height != 2,
                "Incorrect height of root");
        bug_on(map->avl_head->left->height != 1,
                "Incorrect height of left subtree");

        // Linked-list properties
        bug_on(map->areap->start != 0x20000,
                "Linked list start not set");
        bug_on(map->areap->prev,
                "Linked list start has back ref");
        bug_on(map->areap->next->start != 0x40000,
                "Linked list next ptr not updated at first link");
        bug_on(map->areap->next->prev->start != 0x20000,
                "Linked list back refs not updated at first link");
        bug_on(map->areap->next->next,
                "Linked list last link has forward ref");


        vmmap_deinit(map);
}

static void __test
vmmap_test_insert_right(vmmap_t *map, vmobject_t *obj)
{
        vmmap_init(map, proc_current()->control.pmm);

        vmmap_map_object_at(map, obj, 0, 0x40000, PAGE_SIZE);

        bug_on(vmmap_map_object_at(map, obj, 0, 0x60000, PAGE_SIZE),
                        "Mapping failed");
        bug_on(obj->refct != 3, "Refct not bumped");

        // AVL properties
        bug_on(map->avl_head->start != 0x40000,
                "AVL tree head not updated");
        bug_on(map->avl_head->left,
                "Left subtree was set");
        bug_on(map->avl_head->right->start != 0x60000,
                "Right subtree not set");
        bug_on(map->avl_head->right->par->start != 0x40000,
                "Right subtree parent pointer not set");
        bug_on(map->avl_head->right->left || map->avl_head->right->right,
                "Right subtree has children");
        bug_on(map->avl_head->height != 2,
                "Incorrect height of root");
        bug_on(map->avl_head->right->height != 1,
                "Incorrect height of right subtree");

        // Linked-list properties
        bug_on(map->areap->start != 0x40000,
                "Linked list start not set");
        bug_on(map->areap->prev,
                "Linked list start has back ref");
        bug_on(map->areap->next->start != 0x60000,
                "Linked list next ptr not updated at first link");
        bug_on(map->areap->next->prev->start != 0x40000,
                "Linked list back refs not updated at first link");
        bug_on(map->areap->next->next,
                "Linked list last link has forward ref");

        vmmap_deinit(map);
}

static void __test
vmmap_test_insert_rebalance(vmmap_t *map, vmobject_t *obj)
{
        vmmap_init(map, proc_current()->control.pmm);

        vmmap_map_object_at(map, obj, 0, 0x60000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x40000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x20000, PAGE_SIZE);

        // AVL properties
        bug_on(map->avl_head->start != 0x40000,
                "AVL tree head not updated");
        bug_on(!map->avl_head->left ||
                map->avl_head->left->start != 0x20000,
                "Left subtree not set");
        bug_on(!map->avl_head->right ||
                map->avl_head->right->start != 0x60000,
                "Right subtree not set");
        bug_on(!map->avl_head->left->par ||
                map->avl_head->left->par->start != 0x40000,
                "Left subtree parent pointer not set");
        bug_on(!map->avl_head->right->par ||
                map->avl_head->right->par->start != 0x40000,
                "Right subtree parent pointer not set");
        bug_on(map->avl_head->left->left || map->avl_head->left->right,
                "Left subtree has children");
        bug_on(map->avl_head->right->left || map->avl_head->right->right,
                "Right subtree has children");
        bug_on(map->avl_head->height != 2,
                "Incorrect height of root");
        bug_on(map->avl_head->left->height != 1,
                "Incorrect height of left subtree");
        bug_on(map->avl_head->right->height != 1,
                "Incorrect height of right subtree");

        // Linked-list properties
        bug_on(map->areap->start != 0x20000,
                "Linked list start not updated");
        bug_on(map->areap->prev,
                "Linked list start has back ref");
        bug_on(map->areap->next->start != 0x40000,
                "Linked list next ptr not updated at first link");
        bug_on(map->areap->next->prev->start != 0x20000,
                "Linked list back refs not updated at first link");
        bug_on(map->areap->next->next->start != 0x60000,
                "Linked list next ptr not updated at second link");
        bug_on(map->areap->next->next->prev->start != 0x40000,
                "Linked list back refs not updated at second link");
        bug_on(map->areap->next->next->next,
                "Linked list last link has forward ref");

        vmmap_deinit(map);
}

static void __test
vmmap_test_remove(vmmap_t *map, vmobject_t *obj)
{
        vmmap_init(map, proc_current()->control.pmm);

        vmmap_map_object_at(map, obj, 0, 0x40000, PAGE_SIZE);
        vmmap_remove(map, 0x40000, PAGE_SIZE);
        bug_on(obj->refct != 1, "Refct not dropped");
        bug_on(map->avl_head, "AVL tree not unset");
        bug_on(map->areap, "Linked list not unset");

        vmmap_deinit(map);
}

static void __test
vmmap_test_remove_left(vmmap_t *map, vmobject_t *obj)
{
        vmmap_init(map, proc_current()->control.pmm);

        vmmap_map_object_at(map, obj, 0, 0x40000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x60000, PAGE_SIZE);

        vmmap_remove(map, 0x40000, PAGE_SIZE);
        bug_on(obj->refct != 2, "Refct not dropped");

        // AVL properties
        bug_on(!map->avl_head ||
                map->avl_head->start != 0x60000,
                "AVL tree head not updated");
        bug_on(map->avl_head->left || map->avl_head->right,
                "AVL node has children");
        bug_on(map->avl_head->height != 1,
                "Height not updated");

        // Linked-list properties
        bug_on(!map->areap ||
                map->areap->start != 0x60000,
                "Linked list not updated");
        bug_on(map->areap->prev,
                "Linked list has back reference");
        bug_on(map->areap->next,
                "Linked list has forward reference");

        vmmap_deinit(map);
}

static void __test
vmmap_test_remove_right(vmmap_t *map, vmobject_t *obj)
{
        vmmap_init(map, proc_current()->control.pmm);

        vmmap_map_object_at(map, obj, 0, 0x40000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x20000, PAGE_SIZE);

        vmmap_remove(map, 0x40000, PAGE_SIZE);
        bug_on(obj->refct != 2, "Refct not dropped");

        // AVL properties
        bug_on(!map->avl_head ||
                map->avl_head->start != 0x20000,
                "AVL tree head not updated");
        bug_on(map->avl_head->left || map->avl_head->right,
                "AVL node has children");
        bug_on(map->avl_head->height != 1,
                "Height not updated");

        // Linked-list properties
        bug_on(!map->areap ||
                map->areap->start != 0x20000,
                "Linked list not updated");
        bug_on(map->areap->prev,
                "Linked list has back reference");
        bug_on(map->areap->next,
                "Linked list has forward reference");

        vmmap_deinit(map);
}

static void __test
vmmap_test_remove_rebalance(vmmap_t *map, vmobject_t *obj)
{
        vmmap_init(map, proc_current()->control.pmm);

        //       4                   5
        //      / \                 / \
        //     /   \   remove 2    /   \
        //    2     6  =======>   4     6
        //         /
        //        5
        vmmap_map_object_at(map, obj, 0, 0x40000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x20000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x60000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x50000, PAGE_SIZE);

        vmmap_remove(map, 0x20000, PAGE_SIZE);

        // AVL properties
        bug_on(!map->avl_head || map->avl_head->start != 0x50000,
                "AVL tree head not updated");
        bug_on(map->avl_head->height != 2,
                "Root height not updated");
        bug_on(!map->avl_head->left ||
                map->avl_head->left->start != 0x40000,
                "Left node not present");
        bug_on(map->avl_head->left->height != 1,
                "Left height not updated");
        bug_on(map->avl_head->left->left || map->avl_head->left->right,
                "Left subtree has children");
        bug_on(!map->avl_head->right ||
                map->avl_head->right->start != 0x60000,
                "Right node not present");
        bug_on(map->avl_head->right->height != 1,
                "Right height not updated");
        bug_on(map->avl_head->right->left || map->avl_head->right->right,
                "Right subtree has children");

        // Linked-list properties
        bug_on(!map->areap || map->areap->start != 0x40000,
                "Linked list not updated");
        bug_on(map->areap->prev,
                "Linked list has back reference");
        bug_on(!map->areap->next || map->areap->next->start != 0x50000,
                "Linked list first reference not updated");
        bug_on(map->areap->next->prev != map->areap,
                "Linked list first back-ref not updated");
        bug_on(!map->areap->next->next ||
                map->areap->next->next->start != 0x60000,
                "Linked list next reference not updated");
        bug_on(map->areap->next->next->prev != map->areap->next,
                "Linked list next back-ref not updated");
        bug_on(map->areap->next->next->next,
                "Linked list last reference has next ref");

        vmmap_deinit(map);
}

static void __test
vmmap_test_remove_rebalance_multiple(vmmap_t *map, vmobject_t *obj)
{
        vmmap_init(map, proc_current()->control.pmm);

        //          7                  7                     4
        //         / \                / \                   / \
        //        /   \   remove 8   /   \   rebalance     /   \
        //       4     9  =======>  x    10  ========>    2     7-
        //      / \   / \                / \   again     / \   /  \
        //     2   5 8  10              9  11           1   3 5    10
        //    / \   \     \                            /       \   / \
        //   1   3   6    11                          0         6 9  11
        //  /
        // 0
        //
        vmmap_map_object_at(map, obj, 0, 0x70000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x40000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x90000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x20000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x50000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x80000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x100000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x10000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x30000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x60000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x110000, PAGE_SIZE);
        vmmap_map_object_at(map, obj, 0, 0x00000, PAGE_SIZE);

        vmmap_remove(map, 0x80000, PAGE_SIZE);

        // AVL properties
        bug_on(!map->avl_head || map->avl_head->start != 0x40000,
                "AVL tree head not updated");
        bug_on(map->avl_head->height != 4,
                "Root height not updated");
        // Left subtree (which stayed the same, but moved)
        bug_on(!map->avl_head->left ||
                map->avl_head->left->start != 0x20000,
                "L node not present");
        bug_on(map->avl_head->left->height != 3,
                "L height not updated");
        // Right subtree (which was shuffled about, so requires some
        // validation. Ugh.)
        bug_on(!map->avl_head->right ||
                map->avl_head->right->start != 0x70000,
                "R node not present");
        bug_on(map->avl_head->right->height != 3,
                "R height not updated");
        bug_on(!map->avl_head->right->left ||
                map->avl_head->right->left->start != 0x50000,
                "RL not present");
        bug_on(map->avl_head->right->left->height != 2,
                "RL height not updated");
        bug_on(map->avl_head->right->left->left,
                "RL has a left child");
        bug_on(!map->avl_head->right->left->right ||
                map->avl_head->right->left->right->start != 0x60000,
                "RLR not present");
        bug_on(map->avl_head->right->left->right->height != 1,
                "RLR height not updated");
        bug_on(map->avl_head->right->left->right->left ||
                map->avl_head->right->left->right->right,
                "RLR has a child");
        bug_on(!map->avl_head->right->right
                || map->avl_head->right->right->start != 0x100000,
                "RR node not present");
        bug_on(map->avl_head->right->right->height != 2,
                "RR height not updated");
        bug_on(!map->avl_head->right->right->left
                || map->avl_head->right->right->left->start != 0x90000,
                "RRL node not present");
        bug_on(map->avl_head->right->right->left->height != 1,
                "RRL height not updated");
        bug_on(map->avl_head->right->right->left->left
                || map->avl_head->right->right->left->right,
                "RRL node has children");
        bug_on(!map->avl_head->right->right->right
                || map->avl_head->right->right->right->start != 0x110000,
                "RRR node not present");
        bug_on(map->avl_head->right->right->right->height != 1,
                "RRR height not updated");
        bug_on(map->avl_head->right->right->right->left
                || map->avl_head->right->right->right->right,
                "RRR node has children");

        // Linked-list properties
        bug_on(!map->areap || map->areap->start != 0x00000,
                "Linked list not updated");
        vmmap_area_t *p = map->areap;
        vmmap_area_t *prev;
        bug_on(p->prev, "First node has back reference");
        while (p)
        {
                prev = p;
                if (p->next) {
                        bug_on(p->next->prev != p,
                                "Linked list inconsistent");
                        bug_on(p->next->start < p->start,
                                "Linked list out of order");
                        bug_on(p->next->start == 0x80000,
                                "Removed element still linked");
                }
                p = p->next;
        }
        bug_on(prev->next, "Last node has forward reference");
        bug_on(prev->start != 0x110000, "Last node incorrect value");

        vmmap_deinit(map);
}

static void __test
vmmap_test(void)
{
        vmmap_t map;
        vmobject_t *obj = vmobject_create_anon(PAGE_SIZE, PFLAGS_RW);
        obj->refct++; // Prevent the object from getting freed early

        vmmap_test_insert(&map, obj);
        vmmap_test_insert_left(&map, obj);
        vmmap_test_insert_right(&map, obj);
        vmmap_test_insert_rebalance(&map, obj);

        vmmap_test_remove(&map, obj);
        vmmap_test_remove_left(&map, obj);
        vmmap_test_remove_right(&map, obj);
        vmmap_test_remove_rebalance(&map, obj);
        vmmap_test_remove_rebalance_multiple(&map, obj);

        obj->refct--;
        vmobject_destroy(obj);
        kprintf(0, "vmmap_test passed\n");
}

static int
vmmap_global_init(void)
{
        vmmap_area_cache =
                mem_cache_create("vmmap_area_cache", sizeof(vmmap_area_t),
                                 sizeof(vmmap_area_t), 0,
                                 vmmap_area_ctor, vmmap_area_dtor);

        bug_on(!vmmap_area_cache, "Failed to allocate vmmap area cache");
        DO_TEST(vmmap_test);
        return 0;
}

SYSINIT_STEP("vmmap", vmmap_global_init, SYSINIT_VMAP, SYSINIT_VMOBJ);
