/*
 * mm/vma_slab.c - Slab allocator
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 05/16
 */

#include <mm/paging.h>
#include <mm/pfa.h>
#include <mm/vma.h>
#include <mm/vma_slab.h>
#include <sys/bitops_generic.h>
#include <sys/config.h>
#include <sys/kprintf.h>
#include <sys/panic.h>
#include <sys/stdio.h>
#include <sys/string.h>
#include <util/cmp.h>
#include <util/math.h>

typedef struct {
        size_t num_caches;
        struct list_head cache_list;
        /* These contain slab and slab_buf objects for out-of-band
         * slab tracking */
        slab_cache_t slab_cache;
        slab_cache_t slab_buf_cache;
        /* Caches of caches! */
        slab_cache_t cache_cache;
        /* A fixed list of caches for generic allocations. */
        slab_cache_t *kmalloc_caches;
        size_t num_kmalloc_caches;

        struct list_head *reap_scanh; /* Where to start reaping from */
} vma_t;

typedef enum {
        SLAB_STATE_FULL,
        SLAB_STATE_PARTIAL,
        SLAB_STATE_EMPTY,
        INVAL = -1,
} slab_state_t;

typedef struct slab {
        unsigned long    refct;
        unsigned long    num;
        slab_state_t     state;
        struct list_head slab_list;
        struct list_head free_list;
        void            *buf;
} slab_t;

#define SLAB_INIT(slab) {                               \
        .refct = 0,                                     \
        .state = SLAB_STATE_EMPTY,                      \
        .slab_list = LIST_HEAD_INIT((slab).slab_list)   \
}

/* For small objects, we keep these at the start of the memory buffer.
 * For big objects, this is kept as a dynamically allocated external
 * object. */
typedef struct slab_buf {
        void         *buf;
        slab_t       *sp;
        struct list_head free_list;
} slab_buf_t;

#define SLAB_BUF_INIT(slab_buf) {                               \
        .buf = NULL,                                            \
        .cp  = NULL,                                            \
        .free_list = LIST_HEAD_INIT((slab_buf).free_list);      \
}

/* Use ONLY when the slab_buf is in-band (i.e. ~SLAB_CACHE_OFFSLAB) */
#define slab_buf_hdr(buf)  (slab_buf_t *)(buf)
#define slab_buf_data(buf) (void *)((slab_buf_t *)buf + 1)

static void
slab_ctor(void *p, __attribute__((unused)) size_t sz)
{
        slab_t *sp = (slab_t *)p;
        sp->num    = 0;
        sp->refct  = 0;
        sp->state  = SLAB_STATE_EMPTY;
        list_head_init(&sp->slab_list);
        list_head_init(&sp->free_list);
        sp->buf    = NULL;
}

static void
slab_dtor(void *p, __attribute__((unused)) size_t sz)
{
        slab_t *sp = (slab_t *)p;
        bug_on(sp->refct > 0, "Slab freed in use");
        bug_on(sp->num > 0, "Slab freed in use");
}

static void
slab_buf_ctor(void *p, __attribute__((unused)) size_t sz)
{
        slab_buf_t *sp = (slab_buf_t *)p;
        sp->buf = NULL;
        sp->sp = NULL;
        list_head_init(&sp->free_list);
}

static void
slab_buf_dtor(void *p, __attribute__((unused)) size_t sz)
{
        slab_buf_t *sp = (slab_buf_t *)p;
        list_del(&sp->free_list);
}

static void *
slab_data(slab_cache_t *cp, slab_t *sp)
{
        if (cp->flags & SLAB_CACHE_SLABOFF) {
                return sp->buf;
        } else {
                return slab_buf_data(sp->buf);
        }
}

/* This sets up everything except the linked lists and num, which are
 * set up in vma_init_kmalloc_caches(). */
#define KMALLOC_CACHE(sz)                                       \
        {.name ="kmalloc_"#sz,                                  \
         .obj_size = (sz),                                      \
         .pf_order = (sz < PAGE_SIZE ? 2 : 3),                  \
         .align    = (sz),                                      \
         .num      = 0,                                         \
         .refct    = 0,                                         \
         .grown    = 0,                                         \
         .flags    = (sz < PAGE_SIZE/8 ? 0 : SLAB_CACHE_SLABOFF),\
         {NULL, NULL},                                          \
         {NULL, NULL},                                          \
         {NULL, NULL},                                          \
         {NULL, NULL},                                          \
         NULL, NULL                                             \
}

static slab_cache_t malloc_caches[] = {
        KMALLOC_CACHE(4),
        KMALLOC_CACHE(8),
        KMALLOC_CACHE(16),
        KMALLOC_CACHE(32),
        KMALLOC_CACHE(64),
        KMALLOC_CACHE(128),
        KMALLOC_CACHE(256),
        KMALLOC_CACHE(512),
        KMALLOC_CACHE(1024),
        KMALLOC_CACHE(2048),
        KMALLOC_CACHE(4096),
        KMALLOC_CACHE(8192)
};

static vma_t vma = {
        .num_caches         = 0,
        .cache_list         = LIST_HEAD_INIT(vma.cache_list),
        .kmalloc_caches     = malloc_caches,
        .num_kmalloc_caches = sizeof(malloc_caches) / sizeof(slab_cache_t),
        .reap_scanh         = NULL
};

static void
slab_add_cache(slab_cache_t *cp)
{
        list_add(&vma.cache_list, &cp->cache_list);
        vma.num_caches++;
}

static unsigned long
compute_slab_order(size_t obj_size)
{
        unsigned long ret;

        /* TODO: This could be a lot smarter. */
        if (obj_size < PAGE_SIZE / 8)
                ret = 1;
        else if (obj_size < PAGE_SIZE)
                ret = 3;
        else
                ret = MAX(SLAB_MAX_ORDER, 2 + next_pow2(obj_size));

        return ret;
}

static unsigned long
compute_align(slab_cache_t *cp, size_t min_align)
{
        if (min_align < cp->obj_size)
                min_align = cp->obj_size;
        if (cp->flags & SLAB_CACHE_SLABOFF)
                return MAX(cp->obj_size, min_align);
        else
                return MAX(cp->obj_size + sizeof(slab_buf_t), min_align);
}

static unsigned long
compute_num_objs(slab_cache_t *cp)
{
        if (cp->flags & SLAB_CACHE_SLABOFF) {
                return ((PAGE_SIZE << cp->pf_order) / cp->obj_size);
        } else {
                unsigned long size_per_buf
                        = cp->obj_size + sizeof(slab_buf_t);
                return (((PAGE_SIZE << cp->pf_order) - sizeof(slab_t))
                        / size_per_buf);
        }
}

static void
slab_init_cache(slab_cache_t *cp,
                const char *name, size_t size, size_t align,
                slab_cache_flags_t flags,
                void (*ctor)(void *, size_t),
                void (*dtor)(void *, size_t))
{
        strlcpy(cp->name, name, CACHE_NAMELEN+1);
        cp->obj_size            = size;
        cp->pf_order            = compute_slab_order(size);
        cp->align               = compute_align(cp, align);
        cp->flags               = flags;
        cp->num                 = compute_num_objs(cp);
        cp->refct               = 0;
        cp->grown               = 0;
        cp->obj_ctor            = ctor;
        cp->obj_dtor            = dtor;
        list_head_init(&cp->cache_list);
        list_head_init(&cp->slabs_full);
        list_head_init(&cp->slabs_partial);
        list_head_init(&cp->slabs_empty);
}

static void *
slab_getpages(slab_cache_t *cp, mflags_t flags)
{
        void *addr = alloc_pages(flags, cp->pf_order);
        return addr;
}

static void
slab_freepages(slab_cache_t *cp, void *p)
{
        free_pages(p, cp->pf_order);
}

slab_cache_t *
slab_cache_create(const char *name, size_t size, size_t align,
                  slab_cache_flags_t flags,
                  void (*ctor)(void *, size_t),
                  void (*dtor)(void *, size_t))
{
        slab_cache_t *cachep;

        bug_on(flags & ~SLAB_CACHE_GOODFLAGS, "Illegal flags argument.");

        /* For big objects, keep the slab data elsewhere */
        if (size >= PAGE_SIZE / 8)
                flags |= SLAB_CACHE_SLABOFF;

        cachep = (slab_cache_t *)slab_cache_alloc(&vma.cache_cache, M_KERNEL);
        if (!cachep) {
                /* TODO warning */
                return NULL;
        }
        slab_init_cache(cachep, name, size, align, flags, ctor, dtor);
        slab_add_cache(cachep);
        return cachep;
}

static void
slab_destroy(slab_cache_t *cp, slab_t *sp)
{
        unsigned long i;
        void *p = sp->buf;
        /* First of all, destroy each slab object. */
        for (i = 0; i < sp->num; i++)
        {
                if (cp->obj_dtor)
                        cp->obj_dtor(p, cp->obj_size);
                p = (char *)p + cp->align;
        }
        /* Give the slab's pages back to the kernel. */
        slab_freepages(cp, sp->buf);
        /* If we keep book-keeping off-slab, make sure we remove that
         * too. */
        if (cp->flags & SLAB_CACHE_SLABOFF)
                slab_cache_free(&vma.slab_cache, sp);
}

static bool
cache_unused(slab_cache_t *cp)
{
        return (list_empty(&cp->slabs_full) &&
                list_empty(&cp->slabs_partial) &&
                list_empty(&cp->slabs_empty));
}

static void
cache_reap_empty(slab_cache_t *cp)
{
        slab_t *sp, *s;

        list_foreach_entry_safe(&cp->slabs_empty, sp, s, slab_list)
        {
                list_del(&sp->slab_list);
                slab_destroy(cp, sp);
        }
}

int
slab_cache_destroy(slab_cache_t *cp)
{
        bug_on(!cp, "NULL reference");
        bug_on(vma.num_caches == 0, "No caches registered");

        cache_reap_empty(cp);
        if (!cache_unused(cp)) {
                kprintf(PRI_ERR, "Cannot destroy cache (in use)\n");
                return 1;
        }
        if (vma.reap_scanh == &cp->cache_list)
                vma.reap_scanh = cp->cache_list.next;
        slab_cache_free(&vma.cache_cache, cp);
        vma.num_caches--;
        return 0;
}

static int
slab_init_objects(slab_cache_t *cp, slab_t *sp, mflags_t lflags)
{
        void *p;
        unsigned long i;

        p = slab_data(cp, sp);
        for (i = 0; i < cp->num; i++) {
                slab_buf_t *bp;
                void *data = ((char *)p + (i * cp->align));
                if (cp->obj_ctor)
                        cp->obj_ctor(data, cp->obj_size);
                if (!SLAB_CACHE_SLABOFF) {
                        bp = slab_cache_alloc(&vma.slab_buf_cache, lflags);
                        if (!bp)
                                return 1;
                } else {
                        bp = slab_buf_hdr((char *)sp->buf
                                + (i * cp->align));
                        slab_buf_ctor(bp, 0);
                }
                bp->buf = data;
                list_add(&sp->free_list, &bp->free_list);
        }
        return 0;
}

static slab_t *
slab_cache_allocmgmt(slab_cache_t *cp, void *objp, mflags_t lflags)
{
        slab_t *sp;
        if (cp->flags & SLAB_CACHE_SLABOFF) {
               sp = slab_cache_alloc(&vma.slab_cache, lflags);
               if (!sp)
                       return NULL;
        } else {
               sp = (slab_t *)((char *)objp + (cp->num * cp->align));
               slab_ctor(sp, sizeof(slab_t));
        }
        sp->buf = objp;
        if (slab_init_objects(cp, sp, lflags)) {
                slab_dtor(sp, sizeof(slab_t));
                if (cp->flags & SLAB_CACHE_SLABOFF)
                        slab_cache_free(&vma.slab_cache, sp);
                return NULL;
        }
        return sp;
}

static slab_buf_t *
slab_find_free(slab_cache_t *cp, slab_t *sp)
{
        slab_buf_t *bp = list_first_entry_or_null(&sp->free_list,
                                                  slab_buf_t,
                                                  free_list);
        bug_on(!bp, "No free entries in partial slab");
        return bp;
}

void *
slab_cache_alloc(slab_cache_t *cp, mflags_t flags)
{
        slab_buf_t *bp;

        /* First, see if we have a partial slab to use. */
        slab_t *sp = list_first_entry_or_null(&cp->slabs_partial,
                                              slab_t, slab_list);
        if (sp) {
                if (++sp->num == cp->num) {
                        /* Send this over to the full slab list. */
                        list_del(&sp->slab_list);
                        list_add(&cp->slabs_full, &sp->slab_list);
                }
                goto out;
        }

        /* Check the empty list. */
        sp = list_first_entry_or_null(&cp->slabs_empty, slab_t, slab_list);
        if (sp) {
                ++sp->num;
                /* Send this to the partial list. */
                list_del(&sp->slab_list);
                list_add(&cp->slabs_partial, &sp->slab_list);
                /* Let this cache be reaped. */
                cp->grown = 0;
                goto out;
        }

        /* Alas, there are no slabs for us. We have to create a new one. */
        void *buf = slab_getpages(cp, flags);
        if (!buf)
                return NULL;
        sp = slab_cache_allocmgmt(cp, buf, flags);
        if (!sp) {
                slab_freepages(cp, buf);
                return NULL;
        }
        list_add(&cp->slabs_partial, &sp->slab_list);
        /* Prevent reaping until either an alloc happens, or until
         * the slab is a reap candidate twice. */
        cp->grown = 1;
out:
        bp = slab_find_free(cp, sp);
        bug_on(!bp, "No free object found (slab corrupted?)");
        list_del(&bp->free_list);
        return bp->buf;
}

void
slab_cache_free(slab_cache_t *cp, void *obj)
{
        panic("TODO");
}

void
slab_reap(void)
{
        #define REAP_SCANLEN 10 /* Only scan 10 caches per reap */

        slab_cache_t *cp;
        slab_cache_t *to_reap = NULL;
        unsigned int i = 0;
        unsigned int best_num_free;

        if (vma.num_caches == 0)
                return;
        bug_on(vma.reap_scanh == NULL, "VMA in invalid state.");

        /* We scan from the previous entry to the reap_scanh since
         * otherwise we would skip that entry. */
        list_foreach_entry(vma.reap_scanh->prev, cp, cache_list)
        {
                unsigned full_free = 0;
                /* Ensure that we're not scanning the head pointer,
                 * which is in our vma struct and *not* a cache struct. */
                if (&cp->cache_list == &vma.cache_list)
                        continue;
                if (++i > REAP_SCANLEN)
                        break;
                if (cp->flags & SLAB_CACHE_NOREAP)
                        continue;
                if (cp->grown) {
                        cp->grown = 0;
                        continue;
                }

                full_free = list_size(&cp->slabs_empty);
                if (full_free > 0) {
                        /* If there are at least 10 free slabs, just
                         * take this cache and tidy up. */
                        if (full_free > 10) {
                                to_reap = cp;
                                break;
                        }
                        /* Otherwise keep scanning for a better one. */
                        if (full_free > best_num_free) {
                                best_num_free = full_free;
                                to_reap = cp;
                        }
                }
        }
        if (&cp->cache_list == &vma.cache_list)
                vma.reap_scanh = &vma.cache_list;
        else
                vma.reap_scanh = &cp->cache_list;

        if (!to_reap)
                return;
        /* Alright, reap the empty slabs. */
        cache_reap_empty(to_reap);
}

void *
kmalloc(unsigned long size, mflags_t flags)
{
        unsigned long ind;

        if (BAD_MFLAGS(flags) || size == 0)
                return NULL;

        ind = next_pow2(size);
        /* Use the kmalloc_4 slab for 1..4 size allocs */
        if (ind < 2)
                ind = 2;
        if (ind-2 >= vma.num_kmalloc_caches) {
                /* Just go right to the pager. */
                /* TODO book-keeping */
                void *ret = alloc_pages(flags, ind / PAGE_SIZE);
                return ret;

        }
        return slab_cache_alloc(&vma.kmalloc_caches[ind-2], flags);
}

void
kfree(void *addr)
{
        if (!addr)
                return;

        panic("TODO");
}

void
krealloc(void *addr, unsigned long size, mflags_t flags)
{
        if (!addr)
                return;

        panic("TODO");
}

static void
slab_cache_ctor(void *p, __attribute__((unused)) size_t sz)
{
        slab_cache_t *cp = (slab_cache_t *)p;
        cp->refct = 0;
        cp->grown = 0;
        list_head_init(&cp->slabs_full);
        list_head_init(&cp->slabs_partial);
        list_head_init(&cp->slabs_empty);
}

static void
slab_cache_dtor(void *p, __attribute__((unused)) size_t sz)
{
        slab_cache_t *cp = (slab_cache_t *)p;
        bug_on(!list_empty(&cp->slabs_full), "Cache freed in use");
        bug_on(!list_empty(&cp->slabs_partial), "Cache freed in use");
        bzero(cp->name, CACHE_NAMELEN+1);
}

static void
vma_init_kmalloc_caches(void)
{
        unsigned long i;
        for (i = 0; i < vma.num_kmalloc_caches; i++)
        {
                vma.kmalloc_caches[i].num
                        = compute_num_objs(&vma.kmalloc_caches[i]);
                vma.kmalloc_caches[i].align
                        = compute_align(&vma.kmalloc_caches[i], 0);
                list_head_init(&vma.kmalloc_caches[i].cache_list);
                list_head_init(&vma.kmalloc_caches[i].slabs_full);
                list_head_init(&vma.kmalloc_caches[i].slabs_partial);
                list_head_init(&vma.kmalloc_caches[i].slabs_empty);
                slab_add_cache(&vma.kmalloc_caches[i]);
        }
}

static void
vma_init_cache_cache(void)
{
        slab_init_cache(&vma.cache_cache, "cache_cache",
                        sizeof(slab_cache_t), sizeof(slab_cache_t),
                        0, slab_cache_ctor, slab_cache_ctor);
        slab_add_cache(&vma.cache_cache);
        slab_init_cache(&vma.slab_cache, "slab_cache",
                        sizeof(slab_t), sizeof(slab_t),
                        0, slab_ctor, slab_ctor);
        slab_add_cache(&vma.slab_cache);
        slab_init_cache(&vma.slab_buf_cache, "slab_buf_cache",
                        sizeof(slab_buf_t), sizeof(slab_buf_t),
                        0, slab_buf_ctor, slab_buf_ctor);
        slab_add_cache(&vma.slab_buf_cache);
}

void
vma_init(void)
{
        vma_init_cache_cache();
        vma_init_kmalloc_caches();
        bug_on(list_size(&vma.cache_list) != vma.num_caches,
                        "Cache list has incorrect length.\n");
}

void
vma_report(void)
{
        slab_cache_t *cp;
        unsigned long i = 0;
        char buf[54];
        banner(buf, 54, '=', " %3d caches ", vma.num_caches);
        kprintf(0,"%s\n", buf);
        list_foreach_entry_prev(&vma.cache_list, cp, cache_list)
        {
                i++;
                kprintf(0, "%18s: %4d empty %4d partial %4d full\n",
                        cp->name, list_size(&cp->slabs_empty),
                        list_size(&cp->slabs_partial),
                        list_size(&cp->slabs_full));
                bug_on (i > vma.num_caches, "Bad cachelist setup\n");
        }
}

void
vma_test(void)
{
#ifdef CONF_DEBUG
        void *p = NULL;
        int i = -1;
        unsigned int j = -1;
        for (j = 0; j < 8; j++) {
                for (i = 0; i < 100; i++) {
                        p = kmalloc(1 << j, 0);
                        if (p) {
                                j++;
                        }
                }
        }
        kprintf(0, "vma_test passed\n", j);
#endif
}

