#ifndef _MM_RESERVE_H_
#define _MM_RESERVE_H_

#include <stdbool.h>
#include <mm/paging.h>

unsigned long reserve_low_pages(memlimits_t *, unsigned int num);
unsigned long reserve_high_pages(memlimits_t *, unsigned int num);

bool can_reserve();
void disable_reserve();

#endif