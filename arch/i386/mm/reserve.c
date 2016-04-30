#include <stdbool.h>
#include <stddef.h>
#include <mm/paging.h>
#include <mm/reserve.h>
#include <sys/panic.h>

bool reserve_enabled = true;

bool
can_reserve(void)
{
        return reserve_enabled;
}

void
disable_reserve(void)
{
        reserve_enabled = false;
}

unsigned long
reserve_low_pages(memlimits_t *lim, unsigned int num)
{
        unsigned long ret;

        bug_on(!lim, "NULL parameter");
        bug_on(num + lim->low_pfn >= lim->high_pfn,
               "Low memory pool exhausted");
        bug_on(!can_reserve(), "Page reserved after PFA usage");

        if (num == 0)
                return 0;

        ret = paddr_of(lim->low_pfn);
        lim->low_pfn += num;
        return ret;
}

unsigned long
reserve_high_pages(memlimits_t *lim, unsigned int num)
{
        unsigned long ret;

        bug_on(!lim, "NULL parameter");
        bug_on(num + lim->high_pfn >= lim->max_pfn,
               "High memory pool exhausted");
        bug_on(!can_reserve(), "Page reserved after PFA usage");

        if (num == 0)
                return 0;

        ret = paddr_of(lim->high_pfn);
        lim->high_pfn += num;
        return ret;
}
