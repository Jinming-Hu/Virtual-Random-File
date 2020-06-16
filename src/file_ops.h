#pragma once

#define FUSE_USE_VERSION 30
#include <fuse3/fuse.h>
#undef FUSE_USE_VERSION

extern uint64_t file_size;

int vrfs_getattr(const char* path, struct stat* stbuf, fuse_file_info* /* fi */ );
int vrfs_open(const char* /* path */, fuse_file_info* /* fi */);
int vrfs_read(const char* /* path */, char* buf, size_t size, off_t offset, fuse_file_info* /* fi */);
int vrfs_readdir(const char* /* path */, void* buf, fuse_fill_dir_t filler, off_t /* offset */, struct fuse_file_info* /* fi */, enum fuse_readdir_flags /* flags*/);
