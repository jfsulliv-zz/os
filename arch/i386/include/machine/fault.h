#ifndef _MACHINE_FAULT_H_
#define _MACHINE_FAULT_H_

/* Page fault error bits
 *
 * PF_PROT:  0 means no page found, 1 means protection fault
 * PF_WRITE: 0 means read, 1 means write
 * PF_USER:  0 means kernel, 1 means user
 * PF_RSVD:  1 means use of reserved bit detected
 * PF_INSTR: 1 means fault was an instruction fetch
 */
#define PF_PROT  (1<<0)
#define PF_WRITE (1<<1)
#define PF_USER  (1<<2)
#define PF_RSVD  (1<<3)
#define PF_INSTR (1<<4)

void pagefault_handler(struct regs *r);

#endif
