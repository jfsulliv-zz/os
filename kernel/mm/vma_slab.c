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
#include <sys/size.h>
#include <sys/stdio.h>
#include <sys/string.h>
#include <util/cmp.h>
#include <util/math.h>
#include <util/hash.h>

typedef struct {
        size_t num_caches;
        struct list_head cache_list;
        /* These contain slab and slab_buf objects for out-of-band
         * slab tracking. */
        slab_cache_t slab_cache;
        slab_cache_t slab_buf_cache;
        /* Caches of caches! */
        slab_cache_t cache_cache;
        /* A fixed list of caches for generic allocations. */
        slab_cache_t *kmalloc_caches;
        size_t num_kmalloc_caches;
        /* A cache for retaining our kmalloc_record records. */
        slab_cache_t kmalloc_record_cache;
        /* We only use this to attach a number to big kmalloc usage. */
        slab_cache_t kmalloc_big_cache;

        struct list_head *reap_scanh; /* Where to start reaping from */
} vma_t;

typedef enum {
        SLAB_STATE_FULL,
        SLAB_STATE_PARTIAL,
        SLAB_STATE_EMPTY,
        SLAB_STATE_INVAL = -1,
} slab_state_t;

typedef struct slab_buf slab_buf_t;

typedef struct slab {
        unsigned long    refct;
        unsigned long    num;
        slab_state_t     state;
        struct list_head slab_list;
        struct list_head slab_map_list;
        slab_buf_t      *freep;
        slab_buf_t      *lastp;
        slab_buf_t      *slab_bufs;
        void            *buf;
} slab_t;

#define SLAB_INIT(slab) {                               \
        .refct = 0,                                     \
        .state = SLAB_STATE_EMPTY,                      \
        .slab_list = LIST_HEAD_INIT((slab).slab_list),  \
        .slab_map_list = LIST_HEAD_INIT((slab).slab_map_list),  \
        .freep = NULL,                                  \
        .lastp = NULL,                                  \
        .slab_bufs = NULL,                              \
        .buf = NULL,                                    \
}

/* For small objects, we keep these at the start of the memory buffer.
 * For big objects, this is kept as a dynamically allocated external
 * object, at sp->slab_bufs[slab_buf_index(sp, vaddr)]. */
struct slab_buf {
        void         *buf;
        slab_t       *sp;
        slab_buf_t   *next;
        struct list_head slab_map_list;
};

static void
slab_cache_ctor(void *p, __attribute__((unused)) size_t sz)
{
        slab_cache_t *cp = (slab_cache_t *)p;
        cp->refct = 0;
        cp->grown = 0;
        cp->wastage = 0;
        cp->big_bused = 0;
        list_head_init(&cp->cache_list);
        list_head_init(&cp->slabs_full);
        list_head_init(&cp->slabs_partial);
        list_head_init(&cp->slabs_empty);
        int i = 0;
        for (i = 0; i < SLAB_NUM_BUCKETS; i++)
                list_head_init(&cp->slab_map[i]);
}

static void
slab_cache_dtor(void *p, __attribute__((unused)) size_t sz)
{
        slab_cache_t *cp = (slab_cache_t *)p;
        bug_on(!list_empty(&cp->slabs_full), "Cache freed in use");
        bug_on(!list_empty(&cp->slabs_partial), "Cache freed in use");
        bzero(cp->name, CACHE_NAMELEN+1);
        list_del(&cp->cache_list);
}

static struct list_head *
slab_bucket(slab_cache_t *cp, void *vaddr)
{
        uint32_t h = jenkins_hash32(&vaddr, sizeof(void *), 0xfeedbad);
        return &cp->slab_map[h % SLAB_NUM_BUCKETS];
}

static void
add_to_slab_map(slab_cache_t *cp, slab_buf_t *bp)
{
        struct list_head *m = slab_bucket(cp, bp->buf);
        list_add(m, &bp->slab_map_list);
}

/* Assumes that the cache stores slab data out-of-band (SLABOFF) */
static slab_buf_t *
find_slab_buf_off(slab_cache_t *cp, void *vaddr)
{
        slab_buf_t *bp, *n;
        struct list_head *m = slab_bucket(cp, vaddr);
        list_foreach_entry_safe(m, bp, n, slab_map_list)
        {
                if (bp->buf == vaddr) {
                        list_del(&bp->slab_map_list);
                        return bp;
                }
        }
        return NULL;
}

/* Try to find the buffer control object for the given vaddr. */
static slab_buf_t *
find_slab_buf(slab_cache_t *cp, void *vaddr)
{
        if (cp->flags & SLAB_CACHE_SLABOFF) {
                /* Here we need to look up in the cache's hashtable. */
                return find_slab_buf_off(cp, vaddr);
        } else {
                /* This is the easier case, since the buffer control
                 * object is just below the vaddr. */
                return (slab_buf_t *)vaddr - 1;
        }
}

#define SLAB_BUF_INIT(slab_buf) {                               \
        .buf = NULL,                                            \
        .cp  = NULL,                                            \
        .next = NULL,                                           \
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
        list_head_init(&sp->slab_map_list);
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
        sp->next = NULL;
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
         .wastage  = 0,                                         \
         .big_bused= 0,                                         \
         .flags    = (sz < (PAGE_SIZE/8) ? 0 : SLAB_CACHE_SLABOFF),\
         {NULL, NULL},                                          \
         {NULL, NULL},                                          \
         {NULL, NULL},                                          \
         {NULL, NULL},                                          \
         { { NULL, NULL } },                                    \
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

/* We use this to keep track of big kmallocs which go directly to the
 * system pager. */
typedef struct kmalloc_record {
        void *vaddr;
        unsigned int order;
        unsigned int ind;
        struct list_head list;
} kmalloc_record_t;

static void
kmalloc_record_ctor(void *p, __attribute__((unused)) size_t sz)
{
        kmalloc_record_t *bp = (kmalloc_record_t *)p;
        bp->vaddr = 0;
        bp->order = 0;
        bp->ind   = -1;
        list_head_init(&bp->list);
}

static void
kmalloc_record_dtor(void *p, __attribute__((unused)) size_t sz)
{
        kmalloc_record_t *bp = (kmalloc_record_t *)p;
        list_del(&bp->list);
}

#define KMALLOC_NUM_BUCKETS 113
static struct list_head kmalloc_record_buckets[KMALLOC_NUM_BUCKETS];

static struct list_head *
select_kmalloc_record_bucket(void *vaddr)
{
        uint32_t h = jenkins_hash32(&vaddr, sizeof(void *), 0xdeadbeef);
        return &kmalloc_record_buckets[h % KMALLOC_NUM_BUCKETS];
}

static kmalloc_record_t *
find_kmalloc_record(void *vaddr)
{
        struct list_head *buk = select_kmalloc_record_bucket(vaddr);
        kmalloc_record_t *p;
        list_foreach_entry(buk, p, list)
        {
                if (p->vaddr == vaddr)
                        return p;
        }
        return NULL;
}

static void
add_kmalloc_record(kmalloc_record_t *bp)
{
        struct list_head *buk = select_kmalloc_record_bucket(bp->vaddr);
        list_add(buk, &bp->list);
}

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
        if (vma.reap_scanh == NULL)
                vma.reap_scanh = &cp->cache_list;
        vma.num_caches++;
}

/* Sets cp->size, cp->pf_order, cp->num, cp->align.
 * Assumes that cp->flags are set. */
static unsigned long
compute_slab_wastage(slab_cache_t *cp, size_t min_align)
{
        unsigned long wastage = 0;
        unsigned long waste_per_obj = 0;

        /* Size and alignment are easy. */
        if (min_align < cp->obj_size)
                min_align = cp->obj_size;
        if (cp->flags & SLAB_CACHE_SLABOFF) {
                cp->align = MAX(cp->obj_size, min_align);
        } else {
                cp->align = MAX(cp->obj_size + sizeof(slab_buf_t), min_align);
                waste_per_obj = cp->align - cp->obj_size;
        }

        /* Now we can work out the number of objects per cache and its
         * pf_order simultaneously.
         *
         * The goal here is to have cp->num as close to SLAB_MAX_OBJS
         * as possible while still resulting in cp->align being close
         * to a divisor of 1<<(cp->pf_order).
         * */
        unsigned long total_slabsize;
        total_slabsize = SLAB_MAX_OBJS * cp->align;
        if (!(cp->flags & SLAB_CACHE_SLABOFF)) {
                wastage += sizeof(slab_t);
                total_slabsize -= sizeof(slab_t);
        }
        cp->pf_order = MIN(PFA_MAX_PAGE_ORDER,
                           LOG2(total_slabsize / PAGE_SIZE));
        if (cp->flags & SLAB_CACHE_SLABOFF) {
                cp->num = (PAGE_SIZE<<cp->pf_order)/cp->align;
        } else {
                cp->num = ((PAGE_SIZE<<cp->pf_order) - sizeof(slab_t))
                           / cp->align;
        }

        wastage += waste_per_obj * cp->num;
        wastage += (PAGE_SIZE<<cp->pf_order) - (cp->num * cp->align);
        return wastage;
}

static void
slab_init_cache(slab_cache_t *cp,
                const char *name, size_t size, size_t align,
                slab_cache_flags_t flags,
                void (*ctor)(void *, size_t),
                void (*dtor)(void *, size_t))
{
        slab_cache_ctor(cp, 0);
        strlcpy(cp->name, name, CACHE_NAMELEN+1);
        cp->obj_size            = size;
        cp->flags               = flags;
        cp->wastage             = compute_slab_wastage(cp, align);
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
        bug_on(vma.num_caches == 0, "No caches registered");

        if (cp == NULL)
                return 1;

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

        if (cp->flags & SLAB_CACHE_SLABOFF) {
                sp->slab_bufs = slab_cache_alloc(&vma.slab_buf_cache,
                                                 lflags);
                if (!sp->slab_bufs)
                        return 1;
        } else {
                sp->slab_bufs = NULL;
        }

        p = slab_data(cp, sp);
        for (i = 0; i < cp->num; i++) {
                slab_buf_t *bp;
                void *data = ((char *)p + (i * cp->align));
                if (cp->obj_ctor)
                        cp->obj_ctor(data, cp->obj_size);
                if (cp->flags & SLAB_CACHE_SLABOFF) {
                        bp = &sp->slab_bufs[i];
                } else {
                        bp = slab_buf_hdr((char *)sp->buf
                                + (i * cp->align));
                        slab_buf_ctor(bp, 0);
                }
                bp->buf = data;
                bp->sp  = sp;
                if (!sp->freep) {
                        sp->freep = sp->lastp = bp;
                } else {
                        sp->lastp->next = bp;
                        sp->lastp = bp;
                }
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
slab_find_free(slab_t *sp)
{
        return sp->freep;
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
                        sp->state = SLAB_STATE_FULL;
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
                sp->state = SLAB_STATE_PARTIAL;
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
        ++sp->num;
        list_add(&cp->slabs_partial, &sp->slab_list);
        sp->state = SLAB_STATE_PARTIAL;
        /* Prevent reaping until either an alloc happens, or until
         * the slab is a reap candidate twice. */
        cp->grown = 1;
out:
        bp = slab_find_free(sp);
        if (!bp) {
                kprintf(PRI_ERR, "Slab %s has no records!\n", cp->name);
        }
        bug_on(!bp, "No free object found (slab corrupted?)");
        sp->freep = bp->next;
        add_to_slab_map(cp, bp);
        return bp->buf;
}

void
slab_cache_free(slab_cache_t *cp, void *obj)
{
        slab_buf_t *bp = find_slab_buf(cp, obj);
        slab_t *sp;

        if (!bp) {
                kprintf(PRI_ERR, "slab_cache_free: No record found.\n");
                return;
        }
        sp = bp->sp;

        if (sp->num == 0) {
                kprintf(PRI_ERR, "slab_cache_free: Empty slab (double free?)\n");
                return;
        }


        if (cp->obj_dtor)
                cp->obj_dtor(obj, cp->obj_size);

        /* Put the slab buffer object back into the freelist. */
        if (!sp->freep) {
                sp->freep = sp->lastp = bp;
        } else {
                bug_on(!sp->lastp, "Last pointer not set.");
                sp->lastp->next = bp;
                sp->lastp = bp;
                bp->next = NULL;
        }

        /* Make sure we move the slab into the correct list. */
        if (sp->num == cp->num) {
                list_del(&sp->slab_list);
                list_add(&cp->slabs_partial, &sp->slab_list);
                sp->state = SLAB_STATE_PARTIAL;
        }
        if (--sp->num == 0) {
                list_del(&sp->slab_list);
                list_add(&cp->slabs_empty, &sp->slab_list);
                sp->state = SLAB_STATE_EMPTY;
        }
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
                        if (full_free >= best_num_free) {
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

static int
create_kmalloc_record(mflags_t flags, void *vaddr, unsigned long order,
                      unsigned long ind)
{
        kmalloc_record_t *bp = slab_cache_alloc(&vma.kmalloc_record_cache,
                                                flags);
        if (!bp)
                return 1;
        bp->vaddr = vaddr;
        bp->order = order;
        bp->ind   = ind;
        add_kmalloc_record(bp);
        return 0;
}

void *
kmalloc(unsigned long size, mflags_t flags)
{
        unsigned long ind, pf_ord;
        void *ret;

        if (BAD_MFLAGS(flags) || size == 0)
                return NULL;

        ind = next_pow2(size);
        if (size < PAGE_SIZE)
                pf_ord = 0;
        else
                pf_ord = next_pow2(size / PAGE_SIZE);
        /* Use the kmalloc_4 slab for 1..4 size allocs */
        if (ind < 2)
                ind = 2;
        if (ind-2 >= vma.num_kmalloc_caches) {
                /* Just go right to the pager. */
                ret = alloc_pages(flags, pf_ord);
                if (!ret)
                        return NULL;
                if (create_kmalloc_record(flags, ret, pf_ord, ind-2)) {
                        free_pages(ret, ind);
                        return NULL;
                }
                vma.kmalloc_big_cache.big_bused += PAGE_SIZE<<pf_ord;
        } else {
                ret = slab_cache_alloc(&vma.kmalloc_caches[ind-2], flags);
                if (create_kmalloc_record(flags, ret, pf_ord, ind-2)) {
                        slab_cache_free(&vma.kmalloc_caches[ind-2], ret);
                        return NULL;
                }
        }
        return ret;

}

void
kfree(void *addr)
{
        kmalloc_record_t *bp;

        if (!addr)
                return;

        bp = find_kmalloc_record(addr);
        bug_on(!bp, "No previous record found");

        if (bp->ind >= vma.num_kmalloc_caches) {
                free_pages(addr, bp->order);
                bug_on(vma.kmalloc_big_cache.big_bused <
                        (unsigned long)PAGE_SIZE<<bp->order,
                        "Not enough big bytes for freeing.");
                vma.kmalloc_big_cache.big_bused -= PAGE_SIZE<<bp->order;
        } else {
                slab_cache_free(&vma.kmalloc_caches[bp->ind], addr);
        }
        slab_cache_free(&vma.kmalloc_record_cache, bp);
}

void
krealloc(void *addr, unsigned long size, mflags_t flags)
{
        if (!addr)
                return;

        panic("TODO");
}

static void
vma_init_kmalloc_caches(void)
{
        unsigned long i;
        slab_init_cache(&vma.kmalloc_record_cache, "kmalloc_rec_cache",
                        sizeof(kmalloc_record_t), sizeof(kmalloc_record_t),
                        0, kmalloc_record_ctor, kmalloc_record_dtor);
        slab_add_cache(&vma.kmalloc_record_cache);
        for (i = 0; i < vma.num_kmalloc_caches; i++)
        {
                slab_cache_ctor(&vma.kmalloc_caches[i], 0);
                vma.kmalloc_caches[i].wastage
                        = compute_slab_wastage(&vma.kmalloc_caches[i], 0);
                slab_add_cache(&vma.kmalloc_caches[i]);
        }
        for (i = 0; i < KMALLOC_NUM_BUCKETS; i++)
        {
                list_head_init(&kmalloc_record_buckets[i]);
        }
        slab_init_cache(&vma.kmalloc_big_cache, "kmalloc_big",
                        1, 1, 0, NULL, NULL);
        slab_add_cache(&vma.kmalloc_big_cache);
}

static void
vma_init_cache_cache(void)
{
        slab_init_cache(&vma.cache_cache, "cache_cache",
                        sizeof(slab_cache_t), sizeof(slab_cache_t),
                        0, slab_cache_ctor, slab_cache_dtor);
        slab_add_cache(&vma.cache_cache);
        slab_init_cache(&vma.slab_cache, "slab_cache",
                        sizeof(slab_t), sizeof(slab_t),
                        0, slab_ctor, slab_dtor);
        slab_add_cache(&vma.slab_cache);
        slab_init_cache(&vma.slab_buf_cache, "slab_buf_cache",
                        SLAB_MAX_OBJS * sizeof(slab_buf_t),
                        SLAB_MAX_OBJS * sizeof(slab_buf_t),
                        0, slab_buf_ctor, NULL);
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

static inline unsigned long
slab_usage(slab_cache_t *cp, slab_t *sp)
{
        unsigned long sz_per_obj;
        if (cp->flags & SLAB_CACHE_SLABOFF)
                sz_per_obj = cp->obj_size;
        else
                sz_per_obj = cp->obj_size + sizeof(slab_buf_t);
        return sz_per_obj *sp->num;
}

static inline unsigned long
slab_num_records(slab_t *sp)
{
        return sp->num;
}

static inline unsigned long
cache_usage(slab_cache_t *cp)
{
        slab_t *sp;
        unsigned long t = cp->big_bused;
        list_foreach_entry(&cp->slabs_full, sp, slab_list)
        {
               t += slab_usage(cp, sp);
        }
        list_foreach_entry(&cp->slabs_partial, sp, slab_list)
        {
               t += slab_usage(cp, sp);
        }
        return t;
}

static inline unsigned long
cache_num_records(slab_cache_t *cp)
{
        slab_t *sp;
        unsigned long t = 0;
        list_foreach_entry(&cp->slabs_full, sp, slab_list)
        {
               t += slab_num_records(sp);
        }
        list_foreach_entry(&cp->slabs_partial, sp, slab_list)
        {
               t += slab_num_records(sp);
        }
        return t;
}

void
vma_report(void)
{
        slab_cache_t *cp;
        unsigned long i = 0;
        char buf[77];
        banner(buf, sizeof(buf), '=', " %3d caches ", vma.num_caches);
        kprintf(0,"%s\n", buf);
        list_foreach_entry_prev(&vma.cache_list, cp, cache_list)
        {
                i++;
                kprintf(0,
                "%18s: %4d empty %4d partial %4d full |%6d KiB %5d objs\n",
                        cp->name, list_size(&cp->slabs_empty),
                        list_size(&cp->slabs_partial),
                        list_size(&cp->slabs_full),
                        B_KiB(cache_usage(cp)),
                        cache_num_records(cp));
        }
}

static void
vma_test_kmalloc(void)
{
        void *p = NULL;
        int i = -1;
        unsigned int j = -1;
        int n = 0;
        for (j = 0; j < 15; j++) {
                for (i = 0; i < 100; i++) {
                        p = kmalloc(1 << j, 0);
                        if (p) {
                                *(char *)p = 'h'; // Make sure we can read/write
                                kfree(p);
                                n++;
                        }
                }
        }
        kprintf(0, "vma_test_kmalloc passed\n");
}

static void
vma_test_cache_create(void)
{
        slab_cache_t *cp;

        cp = slab_cache_create("test!", 4, 0, 0, NULL, NULL);
        bug_on(!cp, "slab_cache_create failed");

        char *p = slab_cache_alloc(cp, 0);
        bug_on(!p, "slab_cache_alloc returned NULL");
        *p = 'h';
        slab_cache_free(cp, p);

        bug_on (slab_cache_destroy(cp), "Failed to destroy cache");

        kprintf(0, "vma_test_cache_create passed\n");
}

static void
vma_test_reap(void)
{
        slab_reap();
        kprintf(0, "vma_test_reap passed\n");
}

void
vma_test(void)
{
#ifdef CONF_DEBUG
        vma_test_kmalloc();
        vma_test_cache_create();
        vma_test_reap();
#endif
}

