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

#include "fat_impl.h"

#include <fs/fat.h>
#include <sys/error.h>
#include <sys/sysinit.h>
#include <sys/types.h>
#include <util/cmp.h>

#include "fat_dir.h"
#include "fat_utils.h"

ssize_t fat_read(const FatInstance *fat, unsigned int first_cluster,
                 off_t offs, char *buf, size_t num) {
        CHECK_NOTNULL(fat, -EINVAL);
        CHECK_NOTNULL(buf, -EINVAL);

        const unsigned int sector_sz = fat->bpb.le_bytes_per_sector;
        const unsigned int cluster_sz =
                sector_sz * fat->bpb.le_sectors_per_cluster;

        int err;
        uint32_t c = fat_get_cluster(fat, first_cluster, &err);
        CHECK(c != 0, -err, "Failed to read FAT cluster");
        CHECK(!fat_cluster_is_bad(fat, c), -EIO,
                "FAT cluster %ud corrupted", c);

        ssize_t bytes_read = 0;
        while ((size_t)bytes_read < num) {
                size_t to_read = MIN(num - bytes_read, cluster_sz);
                uint32_t addr = fat_first_sector_of_cluster(fat, c);
                if (offs > 0) {
                        const size_t skip = MIN(offs, cluster_sz);
                        offs -= skip;
                        to_read -= skip;
                        addr += skip;
                        if (skip == cluster_sz) {
                                goto next_cluster;
                        }
                }
                // TODO
                //ssize_t res = bio_read(f->dev, buf+bytes_read, addr, to_read);
                ssize_t res = -1;
                CHECK(res >= 0, res, "Failed to read from device");
                bytes_read += res;
next_cluster:
                if ((size_t)bytes_read < num) {
                        if (fat_cluster_is_eoc(fat, c)) {
                                break;
                        }
                        unsigned int cluster = fat_next_cluster(fat, c, &err);
                        CHECK(c > 0, -err, "FAT cluster corrupted: %ux", c);
                        c = fat_get_cluster(fat, cluster, &err);
                        CHECK(c > 0, -err, "Failed to read FAT cluster %ux",
                                cluster);
                }
        }
        return bytes_read;
}

ssize_t fat_write(FatInstance *fat, unsigned int first_cluster,
                  off_t offs, const char *buf, size_t num) {
        CHECK_NOTNULL(fat, -EINVAL);
        CHECK_NOTNULL(buf, -EINVAL);

        const unsigned int sector_sz = fat->bpb.le_bytes_per_sector;
        const unsigned int cluster_sz =
                sector_sz * fat->bpb.le_sectors_per_cluster;

        int err;
        uint32_t c = fat_get_cluster(fat, first_cluster, &err);
        CHECK(c != 0, -err, "Failed to read FAT cluster");
        CHECK(!fat_cluster_is_bad(fat, c), -EIO,
                "FAT cluster %ud corrupted", c);

        ssize_t bytes_written = 0;
        while ((size_t)bytes_written < num) {
                size_t to_write = MIN(num - bytes_written, cluster_sz);
                uint32_t addr = fat_first_sector_of_cluster(fat, c);
                if (offs > 0) {
                        const size_t skip = MIN(offs, cluster_sz);
                        offs -= skip;
                        to_write -= skip;
                        addr += skip;
                        if (skip == cluster_sz) {
                                goto next_cluster;
                        }
                }
                // TODO
                //ssize_t res = bio_write(f->dev, buf+bytes_written, addr, to_write);
                ssize_t res = -1;
                CHECK(res >= 0, res, "Failed to write to device");
                bytes_written += res;
next_cluster:
                if ((size_t)bytes_written < num) {
                        unsigned int cluster;
                        if (fat_cluster_is_eoc(fat, c)) {
                                cluster = fat_alloc_cluster(fat);
                        } else {
                                cluster = fat_next_cluster(fat, c, &err);
                                CHECK(cluster > 0, -err,
                                        "FAT cluster %ux corrupted", cluster);
                        }
                        if (!cluster) {
                                break;
                        }
                        c = fat_get_cluster(fat, cluster, &err);
                        CHECK(c > 0, -err,
                              "FAT cluster %ux corrupted", cluster);
                }
        }
        return bytes_written;
}
