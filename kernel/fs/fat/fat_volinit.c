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

#include <fs/fat.h>
#include <sys/endian.h>
#include <sys/errno.h>
#include <sys/error.h>

#include <stdint.h>

static uint8_t fat_compute_sectors_per_cluster(FatType type, uint32_t disk_size);

int fat_setup_bpb(FatBiosParams *bpb, FatType type, uint32_t disk_size,
                  uint32_t sector_sz)
{
        CHECK_NOTNULL(bpb, EINVAL);
        CHECK(type != FAT_TYPE_12, EINVAL, "Only FAT16/FAT32 supported.\n");
        CHECK(type == FAT_TYPE_16 || type == FAT_TYPE_32, EINVAL,
                "Invalid fat type %d\n", type);

        bpb->le_bytes_per_sector = sector_sz;

        if (type == FAT_TYPE_16) {
                bpb->le_num_sectors_reserved = 1;
                bpb->le_num_fats = 2;
                bpb->le_num_dents = 512;
        } else {
                bpb->le_num_sectors_reserved = 32;
                bpb->le_num_fats = 2;
                // TODO is this right? No reference in docs
                bpb->le_num_dents = 512;
        }
        bpb->le_sectors_per_cluster =
                fat_compute_sectors_per_cluster(type, disk_size);
        CHECK(bpb->le_sectors_per_cluster != 0,
                EINVAL,
                "Invalid disk size %ux for FAT type %d\n",
                disk_size, type);

        uint32_t root_dir_sectors =
                ((bpb->le_num_dents * 32) + bpb->le_bytes_per_sector)
                        / bpb->le_bytes_per_sector;
        uint32_t tmp1 =
                disk_size - (bpb->le_num_sectors_reserved + root_dir_sectors);
        uint32_t tmp2 = (256 * bpb->le_sectors_per_cluster) + bpb->le_num_fats;
        if (type == FAT_TYPE_32)
                tmp2 /= 2;
        uint32_t fat_size = (tmp1 + (tmp2 - 1)) / tmp2;
        if (type == FAT_TYPE_16) {
                bpb->le_sectors_per_fat_16 = (uint16_t)fat_size;
        } else {
                bpb->le_sectors_per_fat_16 = 0;
                bpb->ext.le_sectors_per_fat = fat_size;
        }
        return 0;
}

struct DiskSzToSecPerCluster {
	uint32_t disk_size;
	uint8_t sec_per_cluster;
};

/*
*This is the table for FAT16 drives. NOTE that this table includes
* entries for disk sizes larger than 512 MB even though typically
* only the entries for disks < 512 MB in size are used.
* The way this table is accessed is to look for the first entry
* in the table for which the disk size is less than or equal
* to the disk_size field in that table entry. For this table to
* work properly BPB_RsvdSecCnt must be 1, BPB_NumFATs
* must be 2, and BPB_RootEntCnt must be 512. Any of these values
* being different may require the first table entries disk_size value
* to be changed otherwise the cluster count may be to low for FAT16.
*/
struct DiskSzToSecPerCluster DskTableFAT16[] = {
	{8400, 0}, /* disks up to 4.1 MB, the 0 value for sec_per_cluster
	              trips an error */
	{32680, 2}, /* disks up to 16 MB, 1k cluster */
	{262144, 4}, /* disks up to 128 MB, 2k cluster */
	{524288, 8}, /* disks up to 256 MB, 4k cluster */
	{ 1048576, 16}, /* disks up to 512 MB, 8k cluster */
	/* Entries after this point are not used unless FAT16 is forced */
	{ 2097152, 32}, /* disks up to 1 GB, 16k cluster */
	{ 4194304, 64}, /* disks up to 2 GB, 32k cluster */
	{ 0xFFFFFFFF, 0} /* any disk greater than 2GB, 0 value for
			    sec_per_cluster trips an error */
};

/*
* This is the table for FAT32 drives. NOTE that this table includes
* entries for disk sizes smaller than 512 MB even though typically
* only the entries for disks >= 512 MB in size are used.
* The way this table is accessed is to look for the first entry
* in the table for which the disk size is less than or equal
* to the disk_size field in that table entry. For this table to
* work properly BPB_RsvdSecCnt must be 32, and BPB_NumFATs
* must be 2. Any of these values being different may require the first
* table entries disk_size value to be changed otherwise the cluster count
* may be to low for FAT32.
*/
struct DiskSzToSecPerCluster DskTableFAT32[] = {
	{66600, 0}, /* disks up to 32.5 MB, the 0 value for sec_per_cluster
		       trips an error */
	{532480, 1}, /* disks up to 260 MB, .5k cluster */
	{16777216, 8}, /* disks up to 8 GB, 4k cluster */
	{ 33554432, 16}, /* disks up to 16 GB, 8k cluster */
	{ 67108864, 32}, /* disks up to 32 GB, 16k cluster */
	{ 0xFFFFFFFF, 64}/* disks greater than 32GB, 32k cluster */
};

static uint8_t fat_compute_sectors_per_cluster(FatType type,
                                               uint32_t disk_size)
{
	if (type == FAT_TYPE_16) {
		for (unsigned i = 0; i < sizeof(DskTableFAT16); i++) {
			if (disk_size < DskTableFAT16[i].disk_size)
				return DskTableFAT16[i].sec_per_cluster;
		}
        } else {
		for (unsigned i = 0; i < sizeof(DskTableFAT32); i++) {
			if (disk_size < DskTableFAT32[i].disk_size)
				return DskTableFAT32[i].sec_per_cluster;
		}
	}
	return 0;
}
