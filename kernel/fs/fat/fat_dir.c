#include "fat_dir.h"
#include "fat_utils.h"

#include <fs/fat.h>
#include <sys/error.h>
#include <sys/string.h>
#include <sys/panic.h>

static size_t fat_num_dirents_for_name(const char *name);
static int fat_add_shortdir(const FatInstance *fat, unsigned int parent,
                            const char *name, unsigned int attrs,
                            unsigned int cluster);
static int fat_add_longdir(const FatInstance *fat, unsigned int parent,
                           const char *name, unsigned int attrs,
                           unsigned int cluster);
static int fat_populate_dir(struct fat_instance *fat, unsigned int parent,
                            unsigned int cluster);

static int _fat_create_file(FatInstance *fat, unsigned int parent,
                            const char *name, unsigned int attrs,
                            unsigned int *cluster_num)
{
        *cluster_num = fat_alloc_cluster(fat);
        CHECK(*cluster_num > 0, -ENOSPC,
              "Failed to allocate cluster for dir.");

        int idx = fat_num_dirents_for_name(name) == 1
                ? fat_add_shortdir(fat, parent, name, attrs, *cluster_num)
                : fat_add_longdir(fat, parent, name, attrs, *cluster_num);
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

static const char DOT_ENTRY_NAME[11]    = ".          ";
static const char DOTDOT_ENTRY_NAME[11] = "..         ";

static int fat_populate_dir(struct fat_instance *fat, unsigned int parent,
                            unsigned int cluster)
{
        FatDirEntry dotEntries[2];
        bzero(dotEntries, sizeof(dotEntries));
        memcpy(dotEntries[0].dir.name, DOT_ENTRY_NAME,
               sizeof(dotEntries[0].dir.name));
        memcpy(dotEntries[1].dir.name, DOTDOT_ENTRY_NAME,
               sizeof(dotEntries[1].dir.name));
        dotEntries[0].dir.le_first_cluster_hi = (cluster & 0xFFFF0000) >> 16;
        dotEntries[0].dir.le_first_cluster_lo = (cluster & 0xFFFF);
        dotEntries[1].dir.le_first_cluster_hi = (parent & 0xFFFF0000) >> 16;
        dotEntries[1].dir.le_first_cluster_lo = (parent & 0xFFFF);
        // uint32_t addr = fat_first_sector_of_cluster(fat, cluster);
        // ssize_t res = bio_write(fat->dev, dotEntries, addr, sizeof(dotEntries));
        // CHECK(res == sizeof(dotEntries), EIO, "Failed to write dir contents");
        return 0;
}
