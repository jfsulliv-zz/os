#ifndef __SYSTEM_H_
#define __SYSTEM_H_

/*
 * machine/system.h
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 06/15
 */


/* Read a byte from the target IO port. */
static inline unsigned char inportb(unsigned short _port)
{
        unsigned char rv;
        __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
        return rv;
}


/* Write a byte to the target IO port. */
static inline void outportb(unsigned short _port, unsigned char _data)
{
        __asm__ __volatile__ ("outb %1, %0" : : "dN" (_port), "a" (_data));
}

static inline void io_wait(void)
{
        __asm__ __volatile__ ("outb %%al, $0x80" : : "a"(0) );
}

#endif /* __SYSTEM_H_ */

