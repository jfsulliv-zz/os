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

#ifndef _FAT_FAT_INTERNAL_H_
#define _FAT_FAT_INTERNAL_H_

#include <stdint.h>

// Filesystem metadata sector blob. Stored at a fixed sector offset.
typedef struct fat32_fsinfo_sector {
        uint32_t le_lead_signature; // 0x41615252
        uint8_t reserved1[480];
        uint32_t le_struct_signature; // 0x61417272
        // Last known number of free clusters on the FAT. 0xFFFFFFFF if unknown
        uint32_t le_free_cluster_count;
        // Hint for where to start scanning for free clusters. If 0xFFFFFFF,
        // the free cluster is unknown and the scan should start from cluster 2
        uint32_t le_next_free_cluster;
        uint8_t reserved2[12];
        uint32_t le_tail_signature; // 0xAA550000
} __attribute__((packed)) Fat32FsInfoSector;

#endif
