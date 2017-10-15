/*
Copyright (c) 2017, James Sullivan <sullivan.james.f@gmail.com>
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
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote
      products derived from this software without specific prior
      written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL JAMES SULLIVAN
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "fat_dir.h"
#include "fat_impl.h"
#include "fat_utils.h"

#include <fs/fat.h>
#include <mm/vma.h>
#include <sys/ctype.h>
#include <sys/error.h>
#include <sys/string.h>
#include <sys/panic.h>

static int fat_add_dirent(FatInstance *fat, unsigned int parent,
                          const char *name, unsigned int attrs,
                          unsigned int cluster);
static int fat_populate_dir(struct fat_instance *fat, unsigned int parent,
                            unsigned int cluster);

/* Returns the index of the first newly modified dir entry. */
static int _fat_create_file(FatInstance *fat, unsigned int parent,
                            const char *name, unsigned int attrs,
                            unsigned int *cluster_num)
{
        *cluster_num = fat_alloc_cluster(fat);
        CHECK(*cluster_num > 0, -ENOSPC,
              "Failed to allocate cluster for dir.");

        int idx = fat_add_dirent(fat, parent, name, attrs, *cluster_num);
        if (idx < 0) {
                fat_set_cluster(fat, *cluster_num, 0);
        }
        return idx;
}

int fat_create_file(FatInstance *fat, unsigned int parent, const char *name)
{
        CHECK_NOTNULL(fat, EINVAL);
        CHECK_NOTNULL(name, EINVAL);
        unsigned int cluster_num;
        if (strlen(name) > FAT_MAX_LONG_NAME)
                return EINVAL;
        int idx = _fat_create_file(fat, parent, name, 0, &cluster_num);
        return idx >= 0 ? 0 : -idx;
}

int fat_create_subdir(FatInstance *fat, unsigned int parent, const char *name)
{
        CHECK_NOTNULL(fat, EINVAL);
        CHECK_NOTNULL(name, EINVAL);
        unsigned int cluster_num;
        int idx = _fat_create_file(fat, parent, name, FAT_DIR_ATTR_DIRECTORY,
                                   &cluster_num);
        if (idx < 0) {
                return -idx;
        }
        int ret = fat_populate_dir(fat, parent, cluster_num);
        if (ret) {
                if (fat_remove_dirent(fat, parent, idx)) {
                        kprintf(0, "Failed to clean up failed FAT subdir %s",
                                name);
                }
                return ret;
        }
        return 0;
}

static size_t fat_num_dirents_for_name(const char *name)
{
        size_t len = strlen(name);
        int room_for_dot = strrchr(name, '.') ? 1 : 0;
        if ((len - room_for_dot) < FAT_MAX_SHORT_NAME) {
                // Short name
                return 1;
        }
        // Long name
        return 1 + (len / 14);
}


static FatDirEntry *fat_find_free_dirents(FatDirEntry *buf, size_t num,
                                          size_t wanted) {
        size_t run_len = 0;
        FatDirEntry *run_start = NULL;
        for (FatDirEntry *d = buf; d < buf + num; d++) {
                if (d->dir.name[0] == '\xe5' || d->dir.name[0] == '\x00') {
                        if (!run_start) {
                                run_start = d;
                        }
                        run_len++;
                        if (++run_len == wanted) {
                                return run_start;
                        }
                } else {
                        run_start = NULL;
                        run_len = 0;
                }
        }
        return NULL;
}

static void fat_write_shortname(FatDirEntry *d, const char *name)
{
        // TODO: Handle unicode-encoded strings
        const char *p = name;
        while (*p == ' ' || *p == '.')
                p++;
        int i = 0;
        while (i < 8 && *p && *p != '.') {
                d->dir.name[i++] = toupper(*p);
                p++;
        }
        while (i < 8)
                d->dir.name[i++] = ' ';
        const char *last_dot = strrchr(name, '.');
        if (!last_dot)
                return;
        last_dot++;
        while (i < 11 && *last_dot) {
                d->dir.name[i++] = toupper(*p);
                p++;
        }
}

static void fat_init_shortdir(const FatInstance *fat, FatDirEntry *d,
                              const char *name, unsigned int attrs,
                              unsigned int cluster)
{
        fat_write_shortname(d, name);
        d->dir.attr = attrs;
        d->dir.reserved = 0;
        d->dir.create_time_tenth = 0; // TODO
        d->dir.le_create_time = 0; // TODO
        d->dir.le_access_date = 0; // TODO
        if (fat->type == FAT_TYPE_32) {
                d->dir.le_first_cluster_hi = (cluster & 0xFFFF000) >> 16;
        }
        d->dir.le_write_time = 0; // TODO
        d->dir.le_write_date = 0; // TODO
        d->dir.le_first_cluster_lo = (cluster & 0xFFFF);
        d->dir.le_filesz = 0;
}

static void fat_init_longdir(FatDirEntry *d, const char *name_slice,
                             size_t idx, bool is_last) {
        bug_on(strlen(name_slice) < 26, "FAT string slice is too small");
        bug_on(idx >= FAT_DIR_LAST_LONG_ENTRY, "FAT name is too long.");
        d->long_dir.ord = idx = 1;
        if (is_last) {
                d->long_dir.ord &= FAT_DIR_LAST_LONG_ENTRY;
        }
        strncpy(d->long_dir.name1, name_slice, sizeof(d->long_dir.name1));
        d->long_dir.attr = FAT_DIR_ATTR_LONG_NAME;
        d->long_dir.type = 0;
        strncpy(d->long_dir.name2,
                name_slice + sizeof(d->long_dir.name1),
                sizeof(d->long_dir.name2));
        d->long_dir.always_0 = 0;
        strncpy(d->long_dir.name3,
                name_slice + sizeof(d->long_dir.name1)
                        + sizeof(d->long_dir.name2),
                sizeof(d->long_dir.name3));
        d->long_dir.checksum = fat_longdir_checksum(d);
}

static const char *fat_slice_at(const char *name, size_t idx) {
        const size_t slice_sz = 26;
        bug_on(idx * slice_sz >= strlen(name), "FAT index would overflow");
        return name + (idx * slice_sz);
}

static int fat_add_dirent(FatInstance *fat, unsigned int parent,
                          const char *name, unsigned int attrs,
                          unsigned int cluster)
{
        FatDirEntry *sec_buffer =
                kmalloc(fat->bpb.le_bytes_per_sector, M_KERNEL);
        if (!sec_buffer) return -ENOMEM;
        const size_t ents_per_sector =
                fat->bpb.le_bytes_per_sector / sizeof(FatDirEntry);
        int ret = 0;
        off_t offs = 0;
        const size_t num_entries = fat_num_dirents_for_name(name);
        while (true) {
                ssize_t bytes = fat_read(fat, parent, offs, (char *)sec_buffer,
                                         fat->bpb.le_bytes_per_sector);
                if (bytes < 0) {
                        ret = (int)(bytes);
                        goto out;
                } else if (bytes < fat->bpb.le_bytes_per_sector) {
                        ret = -EIO;
                        goto out;
                }
                FatDirEntry *run =
                        fat_find_free_dirents(sec_buffer, ents_per_sector,
                                              num_entries);
                if (run) {
                        for (FatDirEntry *d = run; d < run + num_entries; d++) {
                                if (d == run + (num_entries - 1)) {
                                        fat_init_shortdir(fat, d, name, attrs,
                                                          cluster);
                                } else {
                                        bug_on(num_entries < (unsigned)(d - run),
                                                "Negative long dir idx");
                                        size_t idx = num_entries - (d - run);
                                        const char *name_slice =
                                                fat_slice_at(name, idx);
                                        bool is_last =
                                                d == run + (num_entries - 2);
                                        fat_init_longdir(d, name_slice, idx,
                                                         is_last);
                                }
                        }
                        fat_write(fat, parent, offs, (char *)sec_buffer,
                                  fat->bpb.le_bytes_per_sector);
                        ret = (run - sec_buffer) + (offs / ents_per_sector);
                        goto out;
                }
                offs += fat->bpb.le_bytes_per_sector;
        }
out:
        kfree(sec_buffer);
        return ret;
}

int fat_remove_dirent(FatInstance *fat, unsigned int parent, unsigned int idx) {
        FatDirEntry *buf =
                kmalloc(fat->bpb.le_bytes_per_sector, M_KERNEL);
        if (!buf) return -ENOMEM;
        const off_t offs = sizeof(FatDirEntry) * idx;
        ssize_t bytes_r = fat_read(fat, parent, offs, (char *)buf,
                                   FAT_MAX_LONG_NAME);
        if (bytes_r < 0) {
                return (int)bytes_r;
        } else if ((bytes_r % sizeof(FatDirEntry) > 0)) {
                return -EIO;
        }
        char empty;
        bool is_last_entries =
                (fat_read(fat, parent, offs + sizeof(FatDirEntry), &empty, 1)
                        < 1);
        size_t num_entries = bytes_r / sizeof(FatDirEntry);
        for (unsigned int i = 0; i < num_entries; i++) {
                if (is_last_entries && i == num_entries - 1) {
                        buf[i].dir.name[0] = 0;
                } else {
                        buf[i].dir.name[0] = 0xE5;
                }
        }
        ssize_t bytes_w = fat_write(fat, parent, offs, (char *)buf, bytes_r);
        if (bytes_w < 0) {
                return (int)bytes_w;
        } else if (bytes_w < bytes_r) {
                return -EIO;
        }
        return 0;
}

static const char DOT_ENTRY_NAME[11]    = ".          ";
static const char DOTDOT_ENTRY_NAME[11] = "..         ";

static int fat_populate_dir(FatInstance *fat, unsigned int parent,
                            unsigned int cluster)
{
        FatDirEntry dot_entries[2];
        bzero(dot_entries, sizeof(dot_entries));
        memcpy(dot_entries[0].dir.name, DOT_ENTRY_NAME,
               sizeof(dot_entries[0].dir.name));
        memcpy(dot_entries[1].dir.name, DOTDOT_ENTRY_NAME,
               sizeof(dot_entries[1].dir.name));
        if (fat->type == FAT_TYPE_32) {
                dot_entries[0].dir.le_first_cluster_hi = (cluster & 0xFFFF0000) >> 16;
                dot_entries[1].dir.le_first_cluster_hi = (parent & 0xFFFF0000) >> 16;
        }
        dot_entries[0].dir.le_first_cluster_lo = (cluster & 0xFFFF);
        dot_entries[1].dir.le_first_cluster_lo = (parent & 0xFFFF);
        // TODO
        // uint32_t addr = fat_first_sector_of_cluster(fat, cluster);
        // ssize_t res = bio_write(fat->dev, dot_entries, addr, sizeof(dot_entries));
        // CHECK(res == sizeof(dot_entries), EIO, "Failed to write dir contents");
        return 0;
}
