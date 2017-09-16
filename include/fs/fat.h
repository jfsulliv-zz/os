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

#ifndef _FS_FAT_H_
#define _FS_FAT_H_

/*
 * fs/fat.h - File Allocation Table
 *
 * Reference: https://staff.washington.edu/dittrich/misc/fatgen103.pdf 
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 04/17
 */

#include <stdint.h>

typedef enum fat_type_enum {
        FAT_TYPE_UNKNOWN = -1,
        FAT_TYPE_12,
        FAT_TYPE_16,
        FAT_TYPE_32
} FatType;

typedef struct fat_extended_boot_record {
        uint8_t drive_number;
        uint8_t nt_flags; // Unused
        uint8_t signature; // 0x28 or 0x29
        uint32_t le_volumeid_serial;
        uint8_t label_string[11]; // Padded with spaces
        uint8_t system_id_string[8];
        uint8_t boot_code[448];
        uint16_t bootable_part_signature; // 0xAA55
} __attribute__((packed)) FatExtendedBootRecord;

typedef struct fat_extended_boot_record_32 {
        uint32_t le_sectors_per_fat;
        uint16_t le_flags;
        uint8_t fat_vernum_minor;
        uint8_t fat_vernum_major;
        uint32_t le_root_dir_cluster_num;
        uint16_t le_fsinfo_sector_num;
        uint16_t le_backup_sector_num;
        uint8_t reserved[12]; // Should be zero
        uint8_t drive_num;
        uint8_t nt_flags; // Unused
        uint8_t signature; // 0x28 or 0x29
        uint32_t le_volumeid_serial;
        uint8_t label_string[11]; // Padded with spaces
        uint8_t system_id_string[8]; // "FAT32   "
        uint8_t boot_code[420];
        uint16_t bootable_part_signature; // 0xAA55
} __attribute__((packed)) FatExtendedBootRecord32;

typedef struct fat_bios_params {
        unsigned char bootjmp[3]; // EB 3C 90
        unsigned char oem_id[8]; // msdosfs
        uint16_t le_bytes_per_sector; // Must be little-endian
        uint8_t le_sectors_per_cluster;
        uint16_t le_num_sectors_reserved;
        uint8_t le_num_fats;
        uint16_t le_num_dents;
        uint16_t le_num_sectors_16;
        uint8_t media_type;
        uint16_t le_sectors_per_fat_16; // FAT12/16 only
        uint16_t le_sectors_per_track;
        uint16_t le_heads_on_media;
        uint32_t le_num_hidden_sectors; // LBA of beginning of partition
        uint32_t le_num_sectors_32; // Unused if num_sectors != 0
        union {
                uint8_t unused[476];
                FatExtendedBootRecord32 ext;
        };
} __attribute__((packed)) FatBiosParams;

/* Instance of a FAT filesystem. */
typedef struct fat_instance {
        FatType type;
        FatBiosParams bpb;
        uint8_t *table; // Should be 1 sector
        unsigned int num_clusters; // Computed at mount-time
} FatInstance;

#endif
