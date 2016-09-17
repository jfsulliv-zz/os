/* Manually curated list of C symbols that need to be used in assembly.
 *
 * There are generally two use cases here:
 * - Accessing C struct members in a safe way
 * - Using #define values
 */

#include <stddef.h>
#include <sys/errno.h>
#include <sys/syscalls.h>

const unsigned ERRNO_ENOSYS = ENOSYS;
const unsigned SYSENT_FUN = offsetof(sysent_t, fun);
const unsigned SYSENT_NUMARGS = offsetof(sysent_t, num_args);
const unsigned ASM_SYS_MAXARGS = SYS_MAXARGS;
const unsigned ASM_SYS_MAXNR = SYS_MAXNR;
