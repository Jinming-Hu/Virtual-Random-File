#define FUSE_USE_VERSION 30
#include <fuse3/fuse.h>
#undef FUSE_USE_VERSION

#include <cstring>
#include <cctype>
#include <cassert>
#include <iostream>
#include <vector>
#include <chrono>


uint64_t file_size;

void set_file_size(const std::string& size_str) {
    std::size_t next_char;
    uint64_t bytes = std::stoull(size_str, &next_char);

    std::string unit = size_str.substr(next_char);
    for (auto& c : unit) c = std::tolower(c);
    if (unit == "b" || unit == "") bytes *= 1;
    else if (unit == "kb" || unit == "k") bytes *= 1024;
    else if (unit == "mb" || unit == "m") bytes *= 1024 * 1024;
    else if (unit == "gb" || unit == "g") bytes *= 1024 * 1024 * 1024;
    else if (unit == "tb" || unit == "t") bytes *= 1024 * 1024 * 1024 * 1024ULL;

    file_size = bytes;
}

int vrfs_getattr(const char* path, struct stat* stbuf, fuse_file_info* /* fi */ ) {
    std::memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0)
        stbuf->st_mode = S_IFDIR | 0755;
    else
        stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = file_size;
    stbuf->st_mtime = 0;

    return 0;
}

int vrfs_open(const char* /* path */, fuse_file_info* /* fi */) {
    return 0;
}

/* __attribute__((always_inline)) */ uint64_t rand_int(uint64_t offset) {
    using uint128_t = __uint128_t;
    constexpr uint128_t mult = static_cast<uint128_t>(0x12e15e35b500f16e) << 64 | 0x2e714eb2b37916a5;
    uint128_t product = offset * mult;
    return product >> 64;
}


int vrfs_read(const char* /* path */, char* buf, size_t size, off_t offset, fuse_file_info* /* fi */) {
    //std::cout << "read " << offset << " " << size << std::endl;
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
    return size;
}

int vrfs_readdir(const char* /* path */, void* buf, fuse_fill_dir_t filler, off_t /* offset */, struct fuse_file_info* /* fi */, enum fuse_readdir_flags /* flags*/) {
    filler(buf, ".", nullptr, 0, fuse_fill_dir_flags(0));
    filler(buf, "..", nullptr, 0, fuse_fill_dir_flags(0));
    filler(buf, "testfile", nullptr, 0, fuse_fill_dir_flags(0));
    return 0;
}

int main(int argc, char** argv) {
    if (argc == 2 && argv[1] == std::string("--test")) {
        // sanity test
        std::string buffer;
        std::string buffer2;
        constexpr std::size_t buffer_size = 8 * 1024 * 1024;
        buffer.resize(buffer_size);
        vrfs_read(nullptr, &buffer[0], buffer.size(), 0, nullptr);
        buffer2 = buffer;
        for (int i = 0; i < 500; ++i) {
            int a = rand() % buffer.size();
            int b = rand() % buffer.size();
            if (a > b) std::swap(a, b);
            std::fill(buffer2.begin() + a, buffer2.begin() + b, '\x00');
            vrfs_read(nullptr, &buffer2[a], b - a, a, nullptr);
            assert(buffer == buffer2);
        }
        return 0;
    } else if (argc == 2 && argv[1] == std::string("--benchmark")) {
        constexpr std::size_t buffer_size = 4 * 1024 * 1024 * 1024ULL;
        std::string buffer;
        buffer.resize(buffer_size);
        for (std::size_t i = 0; i < buffer_size; i += 4096)
            buffer[i] = '\x00';
        auto begin = std::chrono::steady_clock::now();
        vrfs_read(nullptr, &buffer[0], buffer.size(), 0, nullptr);
        auto end = std::chrono::steady_clock::now();
        auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
        std::cout << double(buffer_size / 1024 / 1024 / 1024) / time_ms * 1000 << "GB/s" << std::endl;
        return 0;
    }
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " filesize [fuse options] mountpoint" << std::endl;
        return 0;
    }

    set_file_size(argv[1]);

    std::vector<char*> fuse_args;
    fuse_args.emplace_back(argv[0]);
    std::string fuse_f = "-f";
    fuse_args.emplace_back(&fuse_f[0]);
    for (int i = 2; i < argc; ++i)
        fuse_args.emplace_back(argv[i]);

    struct fuse_operations vrfs_operations;
    memset(&vrfs_operations, 0, sizeof(vrfs_operations));
    vrfs_operations.getattr = vrfs_getattr;
    vrfs_operations.open = vrfs_open;
    vrfs_operations.read = vrfs_read;
    vrfs_operations.readdir = vrfs_readdir;

    return fuse_main(static_cast<int>(fuse_args.size()), fuse_args.data(), &vrfs_operations, nullptr);
}
