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

#include <util/heap.h>
#include <mm/vma.h>

static inline size_t
inplace_parent(size_t ind)
{
        return (ind-1) / 2;
}

static inline size_t
inplace_left(size_t ind)
{
        return (ind * 2) + 1;
}

static inline size_t
inplace_right(size_t ind)
{
        return (ind * 2) + 2;
}

void
inplace_heapify(void *list, size_t num, size_t sz,
                int (*cmp)(const void *, const void *),
                void (*swap)(void *, void *))
{
        size_t start = inplace_parent(num-1);

        if (!list || num == 0 || sz == 0 || !cmp || !swap)
                return;

        while (1)
        {
                inplace_restore(list, sz, start, num-1, cmp, swap);
                if (start == 0)
                        break;
                start--;
        }
}

void
inplace_restore(void *list, size_t sz,
                size_t start, size_t end,
                int (*cmp)(const void *, const void *),
                void (*swap)(void *, void *))
{
        size_t root = start;

        if (!list || sz == 0 || start >= end || !cmp || !swap)
                return;

        while (inplace_left(root) <= end)
        {
                size_t child = inplace_left(root);
                size_t to_swap  = root;

                if (cmp((char *)list + (sz * to_swap), (char *)list + (sz * child)) < 0)
                        to_swap = child;
                /* Prefer to swap a greater right child */
                if (child + 1 <= end &&
                    (cmp((char *)list + (sz * to_swap),
                         (char *)list + (sz * (child + 1))) < 0))
                        to_swap = child+1;

                /* By supposition, the heaps rooted at the child are
                 * valid, so we are done. */
                if (to_swap == root)
                        break;

                swap((char *)list + (sz * root), (char *)list + (sz * to_swap));
                root = to_swap;
        }
}

void
inplace_heapsort(void *list, size_t num, size_t sz,
                 int (*cmp)(const void *, const void *),
                 void (*swap)(void *, void *))
{
        size_t end;

        if (!list || num == 0 || sz == 0 || !cmp || !swap)
                return;

        inplace_heapify(list, num, sz, cmp, swap);

        end = num - 1;
        while (end > 0)
        {
                swap((char *)list + (end * sz), list);
                end--;
                inplace_restore(list, sz, 0, end, cmp, swap);
        }
}

static inline bool
_heap_damaged_up(struct max_heap *h, struct max_heap_node *n)
{
        if (n->parent && h->cmp(n->parent, n) < 0)
                return true;
        return false;
}

static inline bool
_heap_damaged_down(struct max_heap *h, struct max_heap_node *n)
{
        if (n->left && h->cmp(n, n->left) < 0)
                return -1;
        if (n->right && h->cmp(n, n->right) < 0)
                return 1;
        return 0;
}

static struct max_heap_node *
_heap_insert_parent(struct max_heap *heap)
{
        struct max_heap_node *r = heap->root;

        while (1)
        {
                if (r->right) {
                        r = r->right;
                } else if (r->left) {
                        r = r->left;
                } else {
                        break;
                }
        }
        return r;
}

static void
_heap_correct_insert(struct max_heap *heap)
{
        struct max_heap_node *problem = heap->last;
        while (_heap_damaged_up(heap, problem))
        {
                problem = problem->parent;
                heap->swap(problem, problem->parent);
        }
}

static void
_max_heap_insert(struct max_heap *heap, struct max_heap_node *n)
{
        struct max_heap_node *p = _heap_insert_parent(heap);

        if (p->left == NULL)
                p->left = n;
        else
                p->right = n;
        heap->last = n;

        _heap_correct_insert(heap);
}

void
max_heap_insert(struct max_heap *heap, struct max_heap_node *n)
{
        if (!heap || !n)
                return;
        if (!heap->root) {
                heap->root = heap->last = n;
                return;
        }
        _max_heap_insert(heap, n);
}

static void
_heap_correct_take(struct max_heap *heap)
{
        struct max_heap_node *problem = heap->root;
        int lr = 0;
        while ((lr = _heap_damaged_down(heap, problem)) != 0)
        {
                if (lr < 0) {
                        heap->swap(problem, problem->left);
                        problem = problem->left;
                } else {
                        heap->swap(problem, problem->right);
                        problem = problem->right;
                }
        }
}

static void
_heap_remove_last(struct max_heap *h)
{
        struct max_heap_node *rem = h->last;

        if (!rem->parent) {
                h->root = h->last = NULL;
        } else {
                if (rem->parent->left == rem) {
                        rem->parent->left = NULL;
                        h->last = rem->parent;
                } else {
                        rem->parent->right = NULL;
                        h->last = rem->parent->left;
                }
        }
        kfree(rem);
}

void *
max_heap_take(struct max_heap *heap)
{
        void *ret;
        if (!heap || heap->root == NULL)
                return NULL;

        ret = heap->root->val;
        heap->swap(heap->root->val, heap->last->val);
        _heap_correct_take(heap);
        _heap_remove_last(heap);
        return ret;
}

static struct max_heap *
_new_max_heap(size_t size, int (*cmp)(const void *, const void *),
              void (*swap)(void *, void *))
{
        struct max_heap *h = kmalloc(sizeof(struct max_heap), M_KERNEL);
        if (!h)
                return NULL;
        h->root = NULL;
        h->last = NULL;
        h->size = size;
        h->cmp  = cmp;
        h->swap = swap;
        return h;
}

static struct max_heap_node *
_new_node(void *v)
{
        struct max_heap_node *n = kmalloc(sizeof(struct max_heap_node),
                                          M_KERNEL);
        if (!n)
               return NULL;
        n->left = n->right = n->parent = NULL;
        n->val = v;
        return n;
}

struct max_heap *
max_heap_create(size_t size, int (*cmp)(const void *, const void *),
                void (*swap)(void *, void *))
{

        if (!cmp || !swap || size == 0)
                return NULL;
        return _new_max_heap(size, cmp, swap);
}

struct max_heap *
max_heapify(void *list, size_t num, size_t size,
            int (*cmp)(const void *, const void *),
            void (*swap)(void *, void *))
{
        struct max_heap *h;
        size_t i = 0;

        if (!list || num == 0 || size == 0 || !cmp || !swap)
                return NULL;

        h = _new_max_heap(size, cmp, swap);
        if (!h)
                return NULL;

        for (i = 0; i < num; i++)
        {
                struct max_heap_node *n = _new_node((char *)list + (i * size));
                if (!n)
                        goto cleanup;
                max_heap_insert(h, n);
        }
        return h;
cleanup:
        while (max_heap_take(h) != NULL);
        return NULL;
}
