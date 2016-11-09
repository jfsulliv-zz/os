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

// Operations on vnodes.
typedef struct vfs_vnode_ops {
        // Locates a vnode with the given path (relative to the input
        // vnode). If no file is found, returns a non-zero value.
        int (*vnode_find)(vnode_t *, const char *, vnode_t **);
        // Creates a new vnode at the given path (relative to the input
        // vnode). Returns an non-zero value on error (e.g. invalid
        // path, or out of space).
        int (*vnode_create)(vnode_t *, const char *, vnode_t **);
        // Creates a directory at the given path (relative to the input
        // vnode). Returns a non-zero value on error.
        int (*vnode_mkdir)(vnode_t *, const char *, vnode_t **);
        // Removes the directory at the given path (relative to the
        // input vnode). Returns a non-zero value on error.
        int (*vnode_rmdir)(vnode_t *, const char *, vnode_t **);
        // TODO mknod
        // Renames the given vnode (rooted at the first vnode) to the
        // provided name. Returns a non-zero value on error.
        int (*vnode_rename)(vnode_t *, vnode_t *, const char *);
        // TODO link, unlink, symlink, readlink, followlink
} vnode_ops_t;

// Operations on vfiles.
typedef struct vfs_vfile_ops {
        // Sets the seek position of 'vfile'. Returns the new offset
        // on success, or a negative value on error.
        int (*vfile_lseek)(vfile_t *, long, int);
        // Attempts to read a specified number of bytes from 'vfile'.
        // Returns the number of bytes read on success, or a negative
        // value on error.
        int (*vfile_read)(vfile_t *, void *, size_t);
        // Attempts to write a specified number of bytes into 'vfile'.
        // Returns the number of bytes written on success, or a negative
        // value on error.
        int (*vfile_write)(vfile_t *, void *, size_t);
} vfile_ops_t;

#endif
