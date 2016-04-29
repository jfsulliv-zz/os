#ifndef _UTIL_LIST_H_
#define _UTIL_LIST_H_

/*
 * util/list.h: Circular doubly linked list.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 04/16
 */

#include <stdbool.h>
#include <sys/base.h>

struct list_head {
        struct list_head *next;
        struct list_head *prev;
};


static inline void
list_head_init(struct list_head *h)
{
        h->next = h->prev = h;
}

static inline void
__list_insert(struct list_head *bef, struct list_head *n,
              struct list_head *aft)
{
        bef->next = n;
        n->prev   = bef;
        aft->prev = n;
        n->next   = aft;
}

static inline void
list_add(struct list_head *h, struct list_head *n)
{
        __list_insert(h, n, h->next);
}

static inline void
list_add_tail(struct list_head *h, struct list_head *n)
{
        __list_insert(h->prev, n, h);
}

static inline void
list_del(struct list_head *n)
{
        n->prev->next = n->next;
        n->next->prev = n->prev;
}

static inline bool
list_empty(struct list_head *l)
{
        return (l->prev == l->next);
}

#define list_foreach(head, p) \
        for (p = head->next; p != head; p = p->next)

#define list_foreach_prev(head, p) \
        for (p = head->prev; p != head; p = p->prev)

#define list_foreach_safe(head, p, s) \
        for (p = head->next, n = p->next; p != head; p = n, n = n->next)

#define list_foreach_prev_safe(head, p, s) \
        for (p = head->prev, n = p->prev; p != head; p = n, n = n->prev)

#define list_entry(ptr, type, member) \
        container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
        list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
        list_entry((ptr)->prev, type, member)

#define list_first_entry_or_null(ptr, type, member) \
        (!list_empty(ptr) ? list_first_entry(ptr, type, member) : NULL)

#define list_next_entry(pos, member) \
        list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_prev_entry(pos, member) \
        list_entry((pos)->member.prev, typeof(*(pos)), member)

#define list_foreach_entry(head, p, member) \
        for (p = list_first_entry(head, typeof(*p), member);           \
             &p->member != (head);                                     \
             p = list_next_entry(p, member))

#define list_foreach_entry_prev(head, p, member) \
        for (p = list_last_entry(head, typeof(*p), member);            \
             &p->member != (head);                                     \
             p = list_prev_entry(p, member))

#define list_foreach_entry_safe(head, p, s, member) \
        for (p = list_first_entry(head, typeof(*p), member),           \
                s = list_next_entry(p, member);                        \
             &p->member != (head);                                     \
             p = n, n = list_next_entry(n, member))

#define list_foreach_entry_prev_safe(head, p, s, member) \
        for (p = list_last_entry(head, typeof(*p), member),            \
                s = list_prev_entry(p, member);                        \
             &p->member != (head);                                     \
             p = n, n = list_prev_entry(n, member))

#endif
