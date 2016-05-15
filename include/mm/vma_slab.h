#ifndef _MM_VMA_SLAB_H_
#define _MM_VMA_SLAB_H_

#include <sys/bitops_generic.h>
#include <util/list.h>

#define CACHE_NAMELEN 255
#define SLAB_MAX_OBJS 512 /* Max number of objects per slab. */

#define SLAB_CACHE_DMA_BIT      0
#define SLAB_CACHE_NOREAP_BIT   1
#define SLAB_CACHE_SLABOFF_BIT  2

#define SLAB_CACHE_DMA          (1 << SLAB_CACHE_DMA_BIT)
#define SLAB_CACHE_NOREAP       (1 << SLAB_CACHE_NOREAP_BIT)
#define SLAB_CACHE_SLABOFF      (1 << SLAB_CACHE_SLABOFF_BIT)

#define SLAB_CACHE_GOODFLAGS (GENMASK(2,0))

typedef unsigned int slab_cache_flags_t;

typedef struct slab_cache {
        char               name[CACHE_NAMELEN + 1];
        unsigned long      obj_size;
        unsigned long      pf_order;
        unsigned long      align;
        unsigned int       num;
        unsigned long      refct;
        unsigned int       grown;
        unsigned long      wastage;
        unsigned long      big_bused;
        slab_cache_flags_t flags;

        struct list_head   cache_list;
        struct list_head   slabs_full;
        struct list_head   slabs_partial;
        struct list_head   slabs_empty;

#define SLAB_NUM_BUCKETS 113
        struct list_head   slab_map[SLAB_NUM_BUCKETS];

        void (*obj_ctor)(void *, size_t);
        void (*obj_dtor)(void *, size_t);
} slab_cache_t;

slab_cache_t *
slab_cache_create(const char *name, size_t size, size_t align,
                  slab_cache_flags_t flags,
                  void (*ctor)(void *, size_t),
                  void (*dtor)(void *, size_t));

int
slab_cache_destroy(slab_cache_t *cp);

void *
slab_cache_alloc(slab_cache_t *cp, mflags_t flags);

void
slab_cache_free(slab_cache_t *cp, void *obj);

void
slab_reap(void);

#define SLAB_KMALLOC_MAX_ORD 14

#endif
