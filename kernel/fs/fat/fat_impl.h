#ifndef _FS_FAT_FAT_IMPL_H_
#define _FS_FAT_FAT_IMPL_H_

/* Implementation of various FAT operations. Defined internally so that
 * these routines can be used across the FAT implementation and at the
 * front-end. */

#include <stddef.h>
#include <sys/types.h>

struct fat_instance;

ssize_t fat_read(const struct fat_instance *fat, unsigned int first_cluster,
                 off_t offs, char *buf, size_t num);
ssize_t fat_write(struct fat_instance *fat, unsigned int first_cluster,
                  off_t offs, const char *buf, size_t num);

#endif
