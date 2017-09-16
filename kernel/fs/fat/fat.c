/*
Copyright (c) 2016, James Sullivan <sullivan.james.f@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote
      products derived from this software without specific prior
      written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER>
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <fs/fat.h>
#include <fs/vfs.h>
#include <fs/vops.h>
#include <sys/error.h>
#include <sys/sysinit.h>

#include "fat_dir.h"
#include "fat_impl.h"

static ssize_t _fat_read(vnode_t *f, off_t offs, char *buf, size_t num) {
        CHECK_NOTNULL(f, -EINVAL);
        CHECK_NOTNULL(buf, -EINVAL);
        const FatInstance *fat = (FatInstance *)(f->mnt->fs_specific);
        const unsigned int first_cluster = (unsigned int)f->per_fs_data;
        return fat_read(fat, first_cluster, offs, buf, num);
}

static ssize_t _fat_write(vnode_t *f, off_t offs, const char *buf, size_t num) {
        CHECK_NOTNULL(f, -EINVAL);
        CHECK_NOTNULL(buf, -EINVAL);
        FatInstance *fat = (FatInstance *)(f->mnt->fs_specific);
        const unsigned int first_cluster = (unsigned int)f->per_fs_data;
        return fat_write(fat, first_cluster, offs, buf, num);
}


static int _fat_create(vnode_t *base, const char *name) {
        CHECK_NOTNULL(base, EINVAL);
        CHECK_NOTNULL(name, EINVAL);
        FatInstance *fat = (FatInstance *)(base->mnt->fs_specific);
        const unsigned int parent_cluster =
                (const unsigned int)(base->per_fs_data);
        return fat_create_file(fat, parent_cluster, name);
}

static int _fat_mkdir(vnode_t *base, const char *name) {
        CHECK_NOTNULL(base, EINVAL);
        CHECK_NOTNULL(name, EINVAL);
        FatInstance *fat = (FatInstance *)(base->mnt->fs_specific);
        const unsigned int parent_cluster =
                (const unsigned int)(base->per_fs_data);
        return fat_create_subdir(fat, parent_cluster, name);
}

static fs_t fatfs = {
        .type = FS_TYPE_FAT,
        .fs_ops  = {
                .fs_mount = NULL,
                .fs_start = NULL,
                .fs_unmount = NULL,
        },
        .vnode_ops = {
                .vnode_find = NULL,
                .vnode_create = _fat_create,
                .vnode_mkdir = _fat_mkdir,
                .vnode_rmdir = NULL,
                .vnode_rename = NULL,
                .vnode_read = _fat_read,
                .vnode_write = _fat_write,
        },
};
static int fat_fs_init(fs_t *);
REGISTER_FILESYSTEM(fat, &fatfs, FS_TYPE_FAT, fat_fs_init);

static int fat_fs_init(__attribute__((unused)) fs_t *fatfs)
{
        return 0;
}
