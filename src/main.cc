#include <cstring>
#include <cctype>
#include <iostream>
#include <vector>
#include <string>

#include "file_ops.h"


void set_file_size(const std::string& size_str) {
    std::size_t next_char;
    uint64_t bytes = std::stoull(size_str, &next_char);

    std::string unit = size_str.substr(next_char);
    for (auto& c : unit) c = static_cast<char>(std::tolower(c));
    if (unit == "b" || unit == "") bytes *= 1;
    else if (unit == "kb" || unit == "k") bytes *= 1024;
    else if (unit == "mb" || unit == "m") bytes *= 1024 * 1024;
    else if (unit == "gb" || unit == "g") bytes *= 1024 * 1024 * 1024;
    else if (unit == "tb" || unit == "t") bytes *= 1024 * 1024 * 1024 * 1024ULL;

    file_size = bytes;
}

int main(int argc, char** argv) {
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
    std::memset(&vrfs_operations, 0, sizeof(vrfs_operations));
    vrfs_operations.getattr = vrfs_getattr;
    vrfs_operations.open = vrfs_open;
    vrfs_operations.read = vrfs_read;
    vrfs_operations.readdir = vrfs_readdir;

    return fuse_main(static_cast<int>(fuse_args.size()), fuse_args.data(), &vrfs_operations, nullptr);
}
