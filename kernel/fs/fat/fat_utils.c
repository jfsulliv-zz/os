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

#include <stdint.h>

#include <mm/vma.h>
#include <fs/fat.h>
#include <sys/debug.h>
#include <sys/endian.h>
#include <sys/errno.h>
#include <sys/error.h>
#include <sys/limits.h>
#include <sys/string.h>
#include <sys/panic.h>

#include "fat_utils.h"

static uint16_t const FAT_12_CLUSTER_MASK_EVN = 0x0FFF;
static uint16_t const FAT_12_CLUSTER_MASK_EVN_NEG = 0xF000;
static uint16_t const FAT_12_CLUSTER_MASK_ODD = 0x000F;

static uint32_t const FAT_32_CLUSTER_MASK_LO = 0x0FFFFFFF;
static uint32_t const FAT_32_CLUSTER_MASK_HI = 0xF0000000;

static unsigned int fat_num_clusters(const FatBiosParams *bpb);
static uint32_t fat_first_data_sector(const FatInstance *);
static int fat_sync_table(const FatInstance *, uint32_t sector_num);
static uint32_t fat_entry_offset(const FatInstance *fat, uint32_t cluster);
static uint32_t fat_entry_sector(const FatInstance *fat, uint32_t offset,
                                 int *err);

int fat_instance_init(FatInstance *fat, const FatBiosParams *bpb) {
        // TODO sanity check the FAT
        fat->table = kmalloc(bpb->le_bytes_per_sector, M_KERNEL);
        CHECK_NOTNULL(fat->table, ENOMEM);
        memcpy(&fat->bpb, bpb, sizeof(FatBiosParams));
        fat->num_clusters = fat_num_clusters(bpb);
        if (fat->num_clusters < 4085) {
                fat->type = FAT_TYPE_12;
        } else if (fat->num_clusters < 65525) {
                fat->type = FAT_TYPE_16;
        } else {
                fat->type = FAT_TYPE_32;
        }
        return 0;
}

void fat_instance_deinit(struct fat_instance *fat) {
        kfree(fat->table);
}

static uint32_t _fat_get_cluster(const FatInstance *fat,
                unsigned int cluster_num, uint32_t ent_offset,
                int *err) {
        if (fat->type == FAT_TYPE_12) {
                CHECKE(
                      ent_offset < (2 * fat->bpb.le_bytes_per_sector) - 1,
                      0, err, EIO,
                      "Invalid FAT12 sector offset %ux\n", ent_offset);
                uint16_t cluster = *((uint16_t *)(fat->table + ent_offset));
                if (cluster_num & 0x1)
                        return (cluster >> 4);
                else
                        return (cluster & FAT_12_CLUSTER_MASK_EVN);
        } else if (fat->type == FAT_TYPE_16) {
                CHECKE(ent_offset < fat->bpb.le_bytes_per_sector - 1,
                      0, err, EIO,
                      "Invalid FAT16 sector offset %ux\n", ent_offset);
                return *((uint16_t *)(fat->table + ent_offset));
        } else {
                CHECKE(ent_offset < fat->bpb.le_bytes_per_sector - 3,
                      0, err, EIO,
                      "Invalid FAT32 sector offset %ux\n", ent_offset);
                return *((uint32_t *)(fat->table + ent_offset))
                        & FAT_32_CLUSTER_MASK_LO;
        }
}

uint32_t fat_get_cluster(const FatInstance *fat, unsigned int cluster_num,
                         int *err) {
        int local_err;
        if (!err) {
                err = &local_err;
        }
        CHECKE_NOTNULL(fat, 0, err, EINVAL);

        uint32_t ent_offset = fat_entry_offset(fat, cluster_num);
        return _fat_get_cluster(fat, cluster_num, ent_offset, err);
}

static int _fat_set_cluster(FatInstance *fat, unsigned int cluster_num,
                uint32_t value, uint32_t ent_offset) {
        if (fat->type == FAT_TYPE_12) {
                CHECK(ent_offset < (2 * fat->bpb.le_bytes_per_sector) - 1,
                        EIO, "Invalid FAT12 sector offset %ux\n", ent_offset);
                uint16_t *clusterp = (uint16_t *)(fat->table + ent_offset);
                if (cluster_num & 0x1) {
                        value <<= 4;
                        *clusterp = *clusterp & FAT_12_CLUSTER_MASK_ODD;
                } else {
                        value &= FAT_12_CLUSTER_MASK_EVN;
                        *clusterp = *clusterp & FAT_12_CLUSTER_MASK_EVN_NEG;
                }
                *clusterp = *clusterp | (uint16_t)value;
        } else if (fat->type == FAT_TYPE_16) {
                CHECK(ent_offset < fat->bpb.le_bytes_per_sector - 1,
                      EIO, "Invalid FAT16 sector offset %ux\n", ent_offset);
                *(uint16_t *)(fat->table + ent_offset) = (uint16_t)value;
        } else {
                CHECK(ent_offset < fat->bpb.le_bytes_per_sector - 3,
                      EIO, "Invalid FAT16 sector offset %ux\n", ent_offset);
                value &= FAT_32_CLUSTER_MASK_LO;
                uint32_t *clusterp = (uint32_t *)(fat->table + ent_offset);
                *clusterp = (*clusterp & FAT_32_CLUSTER_MASK_HI) | value;
        }
        return 0;
}

int fat_set_cluster(FatInstance *fat, unsigned int cluster_num,
                     uint32_t value) {
        CHECK_NOTNULL(fat, EINVAL);

        int err;
        uint32_t ent_offset = fat_entry_offset(fat, cluster_num);
        uint32_t sec_num = fat_entry_sector(fat, ent_offset, &err);
        CHECK(sec_num > 0, err, "FAT table is in invalid state");

        uint32_t old_value =
                _fat_get_cluster(fat, cluster_num, ent_offset, &err);
        CHECK_ZERO(err);
        err = _fat_set_cluster(fat, cluster_num, value, ent_offset);
        CHECK_ZERO(err);

        err = fat_sync_table(fat, sec_num);
        if (err) {
                _fat_set_cluster(fat, cluster_num, old_value, ent_offset);
        }
        return err;
}

uint32_t fat_first_sector_of_cluster(const FatInstance *fat, unsigned int n)
{
        CHECK_NOTNULL(fat, 0);
        return ((n-2) * fat->bpb.le_sectors_per_cluster)
                + fat_first_data_sector(fat);
}

uint32_t fat_root_dir_sector(const FatInstance *fat)
{
        CHECK_NOTNULL(fat, UINT32_MAX);
        if (fat->type == FAT_TYPE_12 || fat->type == FAT_TYPE_16)
                return fat->bpb.le_num_sectors_reserved +
                        (fat->bpb.le_num_fats * fat->bpb.le_num_sectors_16);
        uint32_t cluster =
                fat->bpb.ext.le_root_dir_cluster_num;
        return fat_first_sector_of_cluster(fat, cluster);
}

bool fat_cluster_is_eoc(const FatInstance *fat, uint32_t cluster)
{
        CHECK_NOTNULL(fat, false);
        if (fat->type == FAT_TYPE_12 || fat->type == FAT_TYPE_16)
                return (cluster >= 0xFF8);
        return (cluster >= 0x0FFFFFF8);
}

unsigned int fat_next_cluster(const FatInstance *fat, uint32_t cluster,
                              int *err)
{
        int local_err;
        if (!err) {
                err = &local_err;
        }
        CHECK_NOTNULL(fat, 0);
        if (fat_cluster_is_eoc(fat, cluster)) {
                *err = EINVAL;
                return 0;
        } else if (fat_cluster_is_bad(fat, cluster)) {
                *err = EIO;
                return 0;
        }
        unsigned int cluster_num;
        switch (fat->type) {
                case FAT_TYPE_12:
                        cluster_num = cluster & 0xFFF;
                        CHECKE(cluster_num <= 0xFEF, 0, err, EIO,
                                "Bad cluster %u", cluster_num);
                        break;
                case FAT_TYPE_16:
                        cluster_num = cluster & 0xFFFF;
                        CHECKE(cluster_num <= 0xFFEF, 0, err, EIO,
                               "Bad cluster %u", cluster_num);
                        break;
                case FAT_TYPE_32:
                        cluster_num = cluster;
                        CHECKE(cluster_num <= 0xFFFFFEF, 0, err, EIO,
                                "Bad cluster %u", cluster_num);
                default:
                        bug("Invalid FAT type");
        }
        return cluster_num;
}


bool fat_cluster_is_bad(const FatInstance *fat, uint32_t cluster)
{
        CHECK_NOTNULL(fat, false);
        if (fat->type == FAT_TYPE_12 || fat->type == FAT_TYPE_16)
                return (cluster == 0xFF7);
        return (cluster == 0x0FFFFFF7);
}

static bool fat_cluster_is_free(const FatInstance *fat, unsigned int num,
                                uint32_t cluster) {
        switch (fat->type) {
                case FAT_TYPE_12:
                        return num & 1
                                ? cluster & FAT_12_CLUSTER_MASK_ODD
                                : cluster & FAT_12_CLUSTER_MASK_EVN;
                case FAT_TYPE_16:
                        return cluster & 0xFFFF;
                case FAT_TYPE_32:
                        return cluster & FAT_32_CLUSTER_MASK_LO;
                default:
                        bug("Invalid FAT type");
        }
}

static void fat_mark_cluster_owned(FatInstance *fat, unsigned int num) {
        fat_set_cluster(fat, num, 0xFFFFFFFF);
}

unsigned int fat_alloc_cluster(FatInstance *fat)
{
        CHECK_NOTNULL(fat, 0);
        const unsigned int max = fat->num_clusters;
        unsigned int cluster = 2;
        for (; cluster < max; cluster++) {
                uint32_t offset = fat_entry_offset(fat, cluster);
                int err;
                uint32_t c = _fat_get_cluster(fat, cluster, offset, &err);
                CHECK(c > 0, err, "Bad FAT entry");
                if (fat_cluster_is_free(fat, cluster, c)) {
                        fat_mark_cluster_owned(fat, cluster);
                        return cluster;
                }
        }
        return 0;
}

static unsigned int fat_num_clusters(const FatBiosParams *bpb) {
        uint32_t root_dir_sectors =
                ((bpb->le_num_dents * 32)
                        + (bpb->le_bytes_per_sector - 1))
                / bpb->le_bytes_per_sector;
        uint32_t fat_sz, total_sectors;
        if (bpb->le_num_sectors_16 != 0) {
                fat_sz = bpb->le_sectors_per_fat_16;
                total_sectors = bpb->le_num_sectors_16;
        } else {
                fat_sz = bpb->ext.le_sectors_per_fat;
                total_sectors = bpb->le_num_sectors_32;
        }
        uint32_t data_sec = total_sectors
                - (bpb->le_num_sectors_reserved
                + (bpb->le_num_fats * fat_sz)
                + root_dir_sectors);
        return data_sec / bpb->le_sectors_per_cluster;
}

static uint32_t fat_first_data_sector(const FatInstance *fat)
{
        uint32_t root_dir_sectors =
                ((fat->bpb.le_num_dents * 32)
                        + (fat->bpb.le_bytes_per_sector - 1))
                / fat->bpb.le_bytes_per_sector;
        uint32_t fat_sz;
        if (fat->bpb.le_num_sectors_16 != 0) {
                fat_sz = fat->bpb.le_sectors_per_fat_16;
        } else {
                fat_sz = fat->bpb.ext.le_sectors_per_fat;
        }
        return fat->bpb.le_num_sectors_reserved
                + (fat_sz * fat->bpb.le_num_fats)
                + root_dir_sectors;
}

static uint32_t fat_entry_offset(const FatInstance *fat,
                unsigned int cluster)
{
        switch (fat->type) {
                case FAT_TYPE_12:
                        return cluster + (cluster / 2);
                case FAT_TYPE_16:
                        return cluster * 2;
                case FAT_TYPE_32:
                        return cluster * 4;
                default:
                        bug("Invalid FAT type");
        }
}

static uint32_t fat_entry_sector(const FatInstance *fat,
                uint32_t offset, int *err) {
        uint32_t fat_sz =
                fat->bpb.le_num_sectors_16 ?: fat->bpb.ext.le_sectors_per_fat;
        uint32_t ret = fat->bpb.le_num_sectors_reserved
                + (offset / fat->bpb.le_bytes_per_sector);
        CHECKE(ret < fat_sz, 0, err, EIO,
                "Out-of-bounds sector %ux (fat size %ux)\n", ret, fat_sz);
        if (fat->type == FAT_TYPE_12 && ret == fat_sz - 1) {
                CHECKE(offset + 1 < fat->bpb.le_bytes_per_sector,
                        0, err, EIO,
                        "FAT12: last entry falls off last sector\n");
        }
        return ret;
}

static int fat_sync_table(const FatInstance *fat, uint32_t sector_num) {
        // write the in-memory version of sector 'num' to fat->dev 
        panic("todo");
}
