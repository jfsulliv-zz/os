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

#ifndef _FS_VFS_H_
#define _FS_VFS_H_

/* Virtual File System - Abstraction layer for filesystems. */

#include <fs/vops.h>

/* Supported filesystems */
typedef enum file_system_type {
        FS_TYPE_UNKNOWN = -1,
        FS_TYPE_FAT,
        FS_TYPE_EXT2,
        FS_TYPE_MAX,
} fs_type_t;

/* Internal representation of file systems. One of these structures
 * is defined for each FS, and is registered into the global list for
 * easy lookup. */
typedef struct file_system {
        fs_type_t type;
        // FS related operations (mounting, unmounting...)
        fs_ops_t fs_ops;
        // Vnode related operations (creation, moving, deletion...)
        vnode_ops_t vnode_ops;
        // Vfile related operations (reading, writing, seeking...)
        vfile_ops_t vfile_ops;
} fs_t;

// Global lookup table. Set up during SYSINIT_FS by the various FS
// registry routines.
const extern fs_t *filesystems[FS_TYPE_MAX];

/* Each filesystem implementation should provide a global init function
 * conforming to:
 *
 *   // Returns 0 on success, non-zero on fatal error
 *   static int init_fn(fs_t *fs);
 *
 * And use the following macro to register the initializer to be
 * executed at runtime. This also registers the FS into the global
 * filesystem lookup table, so even if the global init function is a
 * no-op, this step is mandatory.
 *
 * 'name' is a unique name to call the FS (this is useful for debug logs
 * if the initializer fails). Examples: 'fat16', 'tmpfs'
 *
 * 'fsp' is a reference to a static fs_t object that should be
 * initialized by the init_fn call.
 *
 * 'type' is the FS type (fs_type_t).
 */
#define REGISTER_FILESYSTEM(name, fsp, type, init_fn) \
        static int _register_fs_##name(void) { \
                fs_t **fsreg = (fs_t **)(filesystems); \
                fsreg[type] = fsp; \
                return init_fn(fsp); \
        } \
        SYSINIT_STEP("fs_init_" # name, _register_fs_##name, SYSINIT_FS, \
                     SYSINIT_VMAP | SYSINIT_VMOBJ)

#endif
