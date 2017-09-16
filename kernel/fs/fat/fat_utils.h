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

#ifndef _FAT_FAT_UTILS_H_
#define _FAT_FAT_UTILS_H_

#include <stdbool.h>
#include <stdint.h>

enum fat_type_enum;
struct fat_instance;
struct fat_bios_params;

/* Initializes 'fat' with the given parameters. Returns non-zero on error. */
int fat_instance_init(struct fat_instance *fat,
                      const struct fat_bios_params *bpb);

/* Releases resources held by 'fat'. */
void fat_instance_deinit(struct fat_instance *fat);

/* Initializes bpb based on the given disk size and desired type. Returns
 * a non-zero status on failure.
 * Note that only FAT16/FAT32 are supported. */
int fat_setup_bpb(struct fat_bios_params *bpb, enum fat_type_enum type,
                  uint32_t disk_size, uint32_t sector_sz);

/* Returns the value of the given FAT cluster. 0 is returned on error. */
uint32_t fat_get_cluster(const struct fat_instance *, unsigned int cluster_num,
                         int *err);

/* Writes 'value' to the given FAT cluster. 'value' is truncated if the FAT
 * instance is FAT12/16 (regardless of the endianness of the system,
 * the higher-value bytes will be truncated).
 * Returns non-zero on error. */
int fat_set_cluster(struct fat_instance *, unsigned int cluster_num,
                     uint32_t value);

/* Returns the offset of the first sector for the given cluster. */
uint32_t fat_first_sector_of_cluster(const struct fat_instance *fat,
                                     unsigned int n);

/* Returns the sector number for the root sector. */
uint32_t fat_root_dir_sector(const struct fat_instance *);

/* Check if the given FAT cluster is marked as the end of a cluster chain. */
bool fat_cluster_is_eoc(const struct fat_instance *fat, uint32_t cluster);

/* Returns the number of the next cluster following 'cluster' (or 0 if cluster
 * is EOC). */
unsigned int fat_next_cluster(const struct fat_instance *fat, uint32_t cluster,
                              int *err);

/* Check if the given FAT cluster is marked as a bad cluster. */
bool fat_cluster_is_bad(const struct fat_instance *fat, uint32_t cluster);

/* Computes the number of sectors per cluster given a disk size.
   Used when initializing a FAT volume. */
uint8_t fat_compute_sectors_per_cluster(enum fat_type_enum, uint32_t disk_size);

/* Returns an allocated FAT cluster, or 0 if the FAT is full. */
unsigned int fat_alloc_cluster(struct fat_instance *fat);

#endif
