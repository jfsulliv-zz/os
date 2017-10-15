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

#ifndef _FAT_FAT_DIR_H_
#define _FAT_FAT_DIR_H_

#include <stdint.h>

struct fat_instance;

// Indicates a readonly file.
#define FAT_DIR_ATTR_READ_ONLY 0x01

// Indicates a hidden file.
#define FAT_DIR_ATTR_HIDDEN    0x02

// Indicates a file owned by the OS.
#define FAT_DIR_ATTR_SYSTEM    0x04

// Exactly one 'file' on the volume should have this attribute set, and it
// should be in the root directory. The name of this file is the label for
// the volume. The 'le_first_cluster_{lo,hi}' fields of the dir entry must be
// zero.
#define FAT_DIR_ATTR_VOL_ID    0x08

// Indicates a directory.
#define FAT_DIR_ATTR_DIRECTORY 0x10

// Set whenever a file is created, renamed, or written to, which is useful
// for backup utiliies to know which files have changed.
#define FAT_DIR_ATTR_ARCHIVE   0x20

// Indicates the 'file' is part of a name for another long file.
#define FAT_DIR_ATTR_LONG_NAME \
        (FAT_DIR_ATTR_READ_ONLY \
         | FAT_DIR_ATTR_HIDDEN \
         | FAT_DIR_ATTR_SYSTEM \
         | FAT_DIR_ATTR_VOL_ID)

#define FAT_DIR_ATTR_LONG_NAME_MASK \
        (FAT_DIR_ATTR_READ_ONLY \
         | FAT_DIR_ATTR_HIDDEN \
         | FAT_DIR_ATTR_SYSTEM \
         | FAT_DIR_ATTR_VOL_ID \
         | FAT_DIR_ATTR_DIRECTORY \
         | FAT_DIR_ATTR_ARCHIVE)

// Indicates that a long_dir piece is the last piece in a chain.
#define FAT_DIR_LAST_LONG_ENTRY 0x40

// The largest names supported by each FAT dir type.
#define FAT_MAX_SHORT_NAME 11

// 0x40 is the end marker
#define FAT_MAX_LONG_DIR_CHAIN (0x3F)
#define FAT_MAX_LONG_NAME (FAT_MAX_LONG_DIR_CHAIN * 26)

// Date format: dates are relative to the MS-DOS epoch (01/01/1980).
//      Bits 0-4: Day of month (1-31)
//      Bits 5-8: Month of year (1 = January, 12 = December)
//      Bits 9-15: Years since 1980 (1980-2107)
// Time format: Except for the 'tenth' part, time is in 2-second granularity.
//      Bits 0-4: 2-second count (0-29 or 0-58 seconds)
//      Bits 5-10: Minutes (0-59)
//      Bits 11-15: Hours (0-23)
// The 'tenth' parth adds 0-199 milliseconds.
typedef struct fat_dir_entry {
        union {
        struct {
                // name[0] == 0xE5: free entry.
                // name[0] == 0x00: free entry which is the last allocated.
                // name[0] == 0x05: treat logically as 0xE5 (KANJI lead byte)
                // name[0] cannot be 0x20, and no byte can be in the following:
                //  * 0x00-0x20 (except 0x05)
                //  * 0x22, 0x2A, 0x2B, 0x2C,       0x2E, 0x2F,
                //          0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F
                //                0x5B, 0x5C, 0x5D,
                //                      0x7C
                // Names must be capitalized.
                // Note that name is divided into two whitespace-separated
                // parts,
                // namely the 'filename' and the 'suffix'.
                // e.g. 'foo.bar'      -> 'FOO     BAR'
                //      'a'            -> 'A          '
                //      'prettybg.big' -> 'PRETTYBGBIG'
                char name[11];
                // FAT_DIR_ATTR_*
                uint8_t attr;
                uint8_t reserved; // Set to 0
                uint8_t create_time_tenth; // Millisec creation time (0-199ms)
                uint16_t le_create_time; // Time file was created
                uint16_t le_create_date; // Date file was created
                uint16_t le_access_date; // Date file was accessed
                // High bits of first cluster num. 0 for FAT12/16
                uint16_t le_first_cluster_hi;
                uint16_t le_write_time; // Time file was last written
                uint16_t le_write_date; // Date file was last written
                uint16_t le_first_cluster_lo; // Low bits of first cluster num
                uint32_t le_filesz; // File size in bytes
        } dir __attribute__((packed));
        struct {
                // Order of this entry in a sequence of long_dir entries.
                // If masked with 0x40, this is the last entry.
                uint8_t ord;
                char name1[10]; // Characters 1-5
                uint8_t attr; // Must be FAT_DIR_ATTRS_LONG_NAME
                uint8_t type; // If zero, a sub-component of a longdir
                uint8_t checksum; // checksum of the short dir_entry name
                char name2[12]; // Characters 6-11
                uint16_t always_0;
                char name3[4]; // Characters 12-13
        } long_dir __attribute__((packed));
        };
} __attribute__((packed)) FatDirEntry;

/* Creates a file inside 'parent' named 'name', returning the index of
 * the new entry in the parent's directory table.
 * 'cluster_num' is set to the first cluster of the new file.
 * Returns non-zero on failure. */
int fat_create_file(struct fat_instance *fat,
                    unsigned int parent, const char *name);

/* Like 'fat_create_file', but also does the following:
 *  - Sets up the directory entry with directory attributes, and
 *  - Initializes the new file's contents with '.' and '..'
 * Returns non-zero on failure. */
int fat_create_subdir(struct fat_instance *fat,
                      unsigned int parent, const char *name);

/* Removes the directory entry in 'parent' with index 'target'.
 * Deallocates all clusters held by the directory entry.
 *
 * If 'target' references a long-dir chain, then the rest of the chain after
 * 'target' is also removed.
 * Returns non-zero on failure. */
int fat_remove_dirent(struct fat_instance *fat, unsigned int parent,
                      unsigned int target);

// Computes the checksum field of long_dir.
static inline uint8_t fat_longdir_checksum(FatDirEntry *shortDirEntry)
{
        uint8_t sum = 0;
        for (int len = 11, i = 0; len != 0; len--, i++) {
                sum = ((sum & 1) ? 0x80 : 0)
                        + (sum >> 1) + shortDirEntry->dir.name[i];
        }
        return sum;
}

#endif
