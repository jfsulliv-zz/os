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
#include <sys/string.h>
#include <sys/panic.h>

static uint16_t const FAT_12_CLUSTER_MASK_EVN = 0x0FFF;
static uint16_t const FAT_12_CLUSTER_MASK_EVN_NEG = 0xF000;
static uint16_t const FAT_12_CLUSTER_MASK_ODD = 0x000F;

static uint32_t const FAT_32_CLUSTER_MASK_LO = 0x0FFFFFFF;
static uint32_t const FAT_32_CLUSTER_MASK_HI = 0xF0000000;

static FatType fat_type(const FatBiosParams *bpb);
static uint32_t fat_first_data_sector(FatInstance *);
static int fat_entry_for_cluster(const FatInstance *, unsigned int cluster_num,
                                 uint32_t *sec_nump, uint32_t *ent_offsetp);
static uint32_t fat_read_sector(const FatInstance *, uint32_t n, uint8_t **buf);
static uint32_t fat_write_sector(const FatInstance *, uint32_t n,
                                 const uint8_t *buf, uint32_t num);

void fat_instance_init(FatInstance *fat, const FatBiosParams *bpb) {
        memcpy(&fat->bpb, bpb, sizeof(FatBiosParams));
        fat->type = fat_type(bpb);
}

uint32_t fat_get_cluster(FatInstance *fat, unsigned int cluster_num, int *err) {
        uint32_t sec_num, ent_offset;
        int local_err;
        if (!err) {
                err = &local_err;
        }
        *err = fat_entry_for_cluster(fat, cluster_num,
                                     &sec_num, &ent_offset);
        CHECK(!*err, 0, "Invalid cluster number %d\n", cluster_num);

        uint8_t *sector;
        uint32_t sec_sz = fat_read_sector(fat, sec_num, &sector);
        CHECKE(sec_sz < fat->bpb.le_bytes_per_sector,
                        0, err, EIO,
                        "Partial read from sector %d\n", sec_num);

        if (fat->type == FAT_TYPE_12) {
                CHECKE(
                      ent_offset < (2 * fat->bpb.le_bytes_per_sector) - 1,
                      0, err, EIO,
                      "Invalid FAT12 sector offset %ux\n", ent_offset);
                uint16_t cluster = *((uint16_t *)(&sector[ent_offset]));
                if (cluster_num & 0x1)
                        return (uint32_t)(cluster >> 4);
                else
                        return (uint32_t)(cluster & FAT_12_CLUSTER_MASK_EVN);
        } else if (fat->type == FAT_TYPE_16) {
                CHECKE(ent_offset < fat->bpb.le_bytes_per_sector - 1,
                      0, err, EIO,
                      "Invalid FAT16 sector offset %ux\n", ent_offset);
                return *((uint16_t *)(&sector[ent_offset]));
        } else {
                CHECKE(ent_offset < fat->bpb.le_bytes_per_sector - 3,
                      0, err, EIO,
                      "Invalid FAT32 sector offset %ux\n", ent_offset);
                return *((uint32_t *)(&sector[ent_offset]))
                        & FAT_32_CLUSTER_MASK_HI;
        }
}

void fat_set_cluster(FatInstance *fat, unsigned int cluster_num,
                     uint32_t value, int *err) {
        uint32_t sec_num, ent_offset;
        int local_err;
        if (!err) {
                err = &local_err;
        }
        *err = fat_entry_for_cluster(fat, cluster_num,
                                     &sec_num, &ent_offset);
        CHECKV(!*err, "Invalid cluster number %d\n", cluster_num);

        uint8_t *sector;
        uint32_t sec_sz = fat_read_sector(fat, sec_num, &sector);
        CHECKVE(sec_sz < fat->bpb.le_bytes_per_sector,
                         err, EIO,
                         "Partial read from sector %d\n", sec_num);

        if (fat->type == FAT_TYPE_12) {
                CHECKVE(ent_offset < (2 * fat->bpb.le_bytes_per_sector) - 1,
                      err, EIO,
                      "Invalid FAT12 sector offset %ux\n", ent_offset);
                uint16_t *clusterp = (uint16_t *)(&sector[ent_offset]);
                if (cluster_num & 0x1) {
                        value <<= 4;
                        *clusterp = *clusterp & FAT_12_CLUSTER_MASK_ODD;
                } else {
                        value &= FAT_12_CLUSTER_MASK_EVN;
                        *clusterp = *clusterp & FAT_12_CLUSTER_MASK_EVN_NEG;
                }
                *clusterp = *clusterp | (uint16_t)value;
        } else if (fat->type == FAT_TYPE_16) {
                CHECKVE(ent_offset < fat->bpb.le_bytes_per_sector - 1,
                      err, EIO,
                      "Invalid FAT16 sector offset %ux\n", ent_offset);
                *(uint16_t *)(&sector[ent_offset]) = (uint16_t)value;
        } else {
                CHECKVE(ent_offset < fat->bpb.le_bytes_per_sector - 3,
                      err, EIO,
                      "Invalid FAT16 sector offset %ux\n", ent_offset);
                value &= FAT_32_CLUSTER_MASK_LO;
                uint32_t *clusterp = (uint32_t *)(&sector[ent_offset]);
                *clusterp = (*clusterp & FAT_32_CLUSTER_MASK_HI) | value;
        }

        uint32_t sec_written = fat_write_sector(fat, sec_num, sector, sec_sz);
        CHECKVE(sec_written == sec_sz, err, EIO,
              "Partial write to sector %d (%x of %x)\n",
              sec_num, sec_written, sec_sz);
}

uint32_t fat_first_sector_of_cluster(const FatInstance *fat, unsigned int n)
{
        return ((n-2) * fat->bpb.le_sectors_per_cluster)
                + fat_first_data_sector(fat);
}

bool fat_cluster_is_eoc(const FatInstance *fat, uint32_t cluster)
{
        if (fat->type == FAT_TYPE_12 || fat->type == FAT_TYPE_16)
                return (cluster >= 0xFF8);
        return (cluster >= 0x0FFFFFF8);
}

bool fat_cluster_is_bad(const FatInstance *fat, uint32_t cluster)
{
        if (fat->type == FAT_TYPE_12 || fat->type == FAT_TYPE_16)
                return (cluster == 0xFF7);
        return (cluster == 0x0FFFFFF7);
}

static FatType fat_type(const FatBiosParams *bpb)
{
        uint32_t root_dir_sectors =
                ((bpb->le_num_dents * 32)
                        + (bpb->le_bytes_per_sector - 1))
                / bpb->le_bytes_per_sector;
        uint32_t fat_sz, total_sectors;
        if (bpb->le_num_sectors_16 != 0) {
                fat_sz = bpb->le_sectors_per_fat_16;
                total_sectors = bpb->le_num_sectors_16;
        } else {
                FatExtendedBootRecord32 *ext =
                        (FatExtendedBootRecord32 *)&bpb->ext;
                fat_sz = ext->le_sectors_per_fat;
                total_sectors = bpb->le_num_sectors_32;
        }
        uint32_t data_sec = total_sectors
                - (bpb->le_num_sectors_reserved
                        + (bpb->le_num_fats * fat_sz)
                        + root_dir_sectors);
        uint32_t num_clusters = data_sec / bpb->le_sectors_per_cluster;
        if (num_clusters < 4085) {
                return FAT_TYPE_12;
        } else if (num_clusters < 65525) {
                return FAT_TYPE_16;
        }
        return FAT_TYPE_32;
}

static uint32_t fat_first_data_sector(FatInstance *fat)
{
        uint32_t root_dir_sectors =
                ((fat->bpb.le_num_dents * 32)
                        + (fat->bpb.le_bytes_per_sector - 1))
                / fat->bpb.le_bytes_per_sector;
        uint32_t fat_sz;
        if (fat->bpb.le_num_sectors_16 != 0) {
                fat_sz = fat->bpb.le_sectors_per_fat_16;
        } else {
                FatExtendedBootRecord32 *ext =
                        (FatExtendedBootRecord32 *)&fat->bpb.ext;
                fat_sz = ext->le_sectors_per_fat;
        }
        return fat->bpb.le_num_sectors_reserved
                + (fat_sz * fat->bpb.le_num_fats)
                + root_dir_sectors;
}

static int fat_entry_for_cluster(const FatInstance *fat,
                                 unsigned int cluster_num,
                                 uint32_t *sec_num,
                                 uint32_t *ent_offset) {
        uint32_t fat_sz;
        if (fat->bpb.le_num_sectors_16 != 0) {
                fat_sz = fat->bpb.le_sectors_per_fat_16;
        } else {
                FatExtendedBootRecord32 *ext =
                        (FatExtendedBootRecord32 *)&fat->bpb.ext;
                fat_sz = ext->le_sectors_per_fat;
        }

        uint32_t fat_offset;
        if (fat->type == FAT_TYPE_12)
                fat_offset = cluster_num + (cluster_num / 2);
        else if (fat->type == FAT_TYPE_16)
                fat_offset = cluster_num * 2;
        else
                fat_offset = cluster_num* 4;
        *sec_num = fat->bpb.le_num_sectors_reserved
                + (fat_offset / fat->bpb.le_bytes_per_sector);
        *ent_offset = fat_offset % fat->bpb.le_bytes_per_sector;
        CHECK(*sec_num >= fat_sz, EIO,
              "Out-of-bounds sector %ux (fat size %ux)\n",
              sec_num, fat_sz);
        CHECK(fat->type == FAT_TYPE_12
                        && *sec_num == fat_sz - 1
                        && *ent_offset + 1 >= fat->bpb.le_bytes_per_sector,
              EIO, "FAT12: sector %ux is last sector\n", sec_num);
        return 0;
}

static uint32_t fat_read_sector(const FatInstance *fat, uint32_t n,
                                uint8_t **buf) {
        panic("TODO");
}

static uint32_t fat_write_sector(const FatInstance *fat, uint32_t n,
                                 const uint8_t *buf, uint32_t num) {
        panic("TODO");
}
