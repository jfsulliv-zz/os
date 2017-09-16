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

#ifndef _FS_VOPS_H_
#define _FS_VOPS_H_

/*
 * fs/vops.h - Filesystem virtual operations.
 *
 * Each filesystem implementation must provide a vops table that the VFS
 * calls into to perform common file operations.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 04/17
 */
#include <fs/mount.h>
#include <fs/vnode.h>
#include <fs/vfile.h>
#include <sys/types.h>

#include <stddef.h>

// Operations on filesystems (mount points).
typedef struct vfs_fs_ops {
        // Mounts the filesystem at the given path. Returns non-zero
        // on error.
        int (*fs_mount)(mount_t *, const char *);
        // Initializes a mounted filesystem. Returns non-zero on error.
        int (*fs_start)(mount_t *, int flags);
        // Unmounts a previously mounted filesystem. Returns non-zero
        // on error.
        int (*fs_unmount)(mount_t *);
} fs_ops_t;

// Operations on vnodes, which represent a single entity in a filesystem.
typedef struct vfs_vnode_ops {
        // Locates a vnode with the given path (relative to the input
        // vnode). If no file is found, returns a non-zero value.
        int (*vnode_find)(const vnode_t *base, const char *, vnode_t **);
        // Creates a new file with the given name as a child of 'base'.
        // Returns an non-zero value on error (e.g. invalid name, or out of
        // space).
        int (*vnode_create)(vnode_t *base, const char *);
        // Creates a directory at the given path (relative to the input
        // vnode). Returns a non-zero value on error.
        int (*vnode_mkdir)(vnode_t *base, const char *);
        // Removes the directory at the given path (relative to the
        // input vnode). Returns a non-zero value on error.
        int (*vnode_rmdir)(vnode_t *base, const char *);
        // TODO mknod
        // Renames the given vnode (rooted at the first vnode) to the
        // provided name. Returns a non-zero value on error.
        int (*vnode_rename)(vnode_t *base, vnode_t *, const char *);
        // Attempts to read a specified number of bytes from 'vnode'.
        // Returns the number of bytes read on success, or a negative
        // value on error.
        ssize_t (*vnode_read)(vnode_t *, off_t offs, char *buf, size_t sz);
        // Attempts to write a specified number of bytes into 'vnode'.
        // Returns the number of bytes written on success, or a negative
        // value on error.
        ssize_t (*vnode_write)(vnode_t *, off_t offs, const char *buf, size_t);
        // TODO link, unlink, symlink, readlink, followlink, stat
} vnode_ops_t;

#endif
