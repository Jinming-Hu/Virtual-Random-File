#include "file_ops.h"

#ifdef _WIN32
#include <intrin.h>
#include <sys/stat.h>
#endif

#include <cstring>
#include <iostream>
#include <algorithm>


uint64_t file_size = 0;

int vrfs_getattr(const char* path, fuse_stat* stbuf, fuse_file_info* /* fi */) {
    std::memset(stbuf, 0, sizeof(*stbuf));

    if (strcmp(path, "/") == 0)
        stbuf->st_mode = S_IFDIR | 0755;
    else
        stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = file_size;
#ifdef _WIN32
    stbuf->st_mtim.tv_sec = 0;
    stbuf->st_mtim.tv_nsec = 0;
#else
    stbuf->st_mtime = 0;
#endif
    return 0;
}

int vrfs_open(const char* /* path */, fuse_file_info* /* fi */) {
    return 0;
}

uint64_t rand_int(uint64_t offset) {
#ifdef _WIN32
    uint64_t high_part = 0x12e15e35b500f16e;
    uint64_t low_part = 0x2e714eb2b37916a5;
    return __umulh(low_part, offset) + high_part * offset;
#else
    using uint128_t = __uint128_t;
    constexpr uint128_t mult = static_cast<uint128_t>(0x12e15e35b500f16e) << 64 | 0x2e714eb2b37916a5;
    uint128_t product = offset * mult;
    return product >> 64;
#endif
}

int vrfs_read(const char* /* path */, char* buf, size_t size, fuse_off_t offset, fuse_file_info* /* fi */) {
    size = std::min(size, static_cast<size_t>(std::numeric_limits<int>::max()));

    constexpr std::size_t int_size = sizeof(uint64_t);

    const uint64_t org_offset = offset;
    uint64_t gen_offset = org_offset / int_size * int_size;
    const uint64_t end_offset = org_offset + size;
    if (gen_offset < org_offset) {
        uint64_t r = rand_int(gen_offset);
        gen_offset += int_size;
        std::memcpy(buf, reinterpret_cast<char*>(&r) + org_offset % int_size, gen_offset - org_offset);
        buf += gen_offset - org_offset;
    }
    while (gen_offset + int_size <= end_offset) {
        *reinterpret_cast<uint64_t*>(buf) = rand_int(gen_offset);
        buf += int_size;
        gen_offset += int_size;
    }
    if (gen_offset < end_offset) {
        uint64_t r = rand_int(gen_offset);
        std::memcpy(buf, reinterpret_cast<char*>(&r), end_offset - gen_offset);
        buf += end_offset - gen_offset;
    }

    return static_cast<int>(size);
}

int vrfs_readdir(const char* /* path */, void* buf, fuse_fill_dir_t filler, fuse_off_t /* offset */, struct fuse_file_info* /* fi */, fuse_readdir_flags /* flags*/) {
    filler(buf, ".", nullptr, 0, fuse_fill_dir_flags(0));
    filler(buf, "..", nullptr, 0, fuse_fill_dir_flags(0));
    filler(buf, "testfile", nullptr, 0, fuse_fill_dir_flags(0));
    return 0;
}

