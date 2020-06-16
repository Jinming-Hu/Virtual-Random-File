#include "unistd.h"
#include <sys/mman.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

#include "file_ops.h"


constexpr std::size_t buffer_size = 4 * 1024 * 1024 * 1024ULL;
constexpr std::size_t page_size = 4096;

void vrfs_read_test() {
    constexpr std::size_t page_size = 4096;
    std::string buffer;
    buffer.resize(buffer_size);
    for (std::size_t i = 0; i < buffer_size; i += page_size)
       buffer[i] = '\x00';
    auto begin = std::chrono::steady_clock::now();
    vrfs_read(nullptr, &buffer[0], buffer.size(), 0, nullptr);
    auto end = std::chrono::steady_clock::now();
    auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "vrfs_read: " << double(buffer_size / 1024 / 1024 / 1024) / time_ms * 1000 << "GB/s" << std::endl;
}

void fin_read_test(const std::string& file) {
    std::ifstream fin(file, std::ifstream::binary);
    std::string buffer;
    buffer.resize(buffer_size);
    auto begin = std::chrono::steady_clock::now();
    fin.read(&buffer[0], buffer.size());
    auto end = std::chrono::steady_clock::now();
    if (fin.gcount() != buffer_size) std::abort();
    auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "fin_read: " << double(buffer_size / 1024 / 1024 / 1024) / time_ms * 1000 << "GB/s" << std::endl;
}

void fread_test(const std::string& file) {
    FILE* fin = fopen(file.data(), "rb");
    if (!fin) std::abort();
    std::string buffer;
    buffer.resize(buffer_size);
    auto begin = std::chrono::steady_clock::now();
    std::size_t num_read = fread(&buffer[0], buffer.size(), 1, fin);
    auto end = std::chrono::steady_clock::now();
    if (num_read != 1) std::abort();
    auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "fread: " << double(buffer_size / 1024 / 1024 / 1024) / time_ms * 1000 << "GB/s" << std::endl;
    fclose(fin);
}

void direct_read_test(const std::string& file) {
    int fd = open(file.data(), O_RDONLY | O_DIRECT);
    if (fd <= 0) std::abort();
    std::string buffer;
    buffer.resize(buffer_size);
    auto begin = std::chrono::steady_clock::now();
    std::size_t num_read = 0;
    while (num_read < buffer_size) {
        ssize_t chunk_read = read(fd, &buffer[num_read], buffer_size - num_read);
        num_read += chunk_read;
    }
    auto end = std::chrono::steady_clock::now();
    if (num_read != buffer_size) std::abort();
    auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "direct read: " << double(buffer_size / 1024 / 1024 / 1024) / time_ms * 1000 << "GB/s" << std::endl;
    close(fd);
}

void mmap_read_test(const std::string& file) {
    int fd = open(file.data(), O_RDONLY);
    if (fd <= 0) std::abort();

    auto begin = std::chrono::steady_clock::now();
    void* addr = mmap(nullptr, buffer_size, PROT_READ, MAP_PRIVATE, fd, 0);
    const char* buffer = reinterpret_cast<const char*>(addr);
    int donot_optmise = 0;
    for (std::size_t i = 0; i < buffer_size; i += page_size) {
       char c = buffer[i];
       donot_optmise ^= int(c);
    }
    auto end = std::chrono::steady_clock::now();
    if (donot_optmise == 0x1234) std::this_thread::yield();
    auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "mmap read: " << double(buffer_size / 1024 / 1024 / 1024) / time_ms * 1000 << "GB/s" << std::endl;

    munmap(addr, buffer_size);
}

int main(int argc, char** argv) {
    if (argc == 1) {
        std::cout << "Usage: " << argv[0] << " file" << std::endl;
        return 0;
    }
    std::string file = argv[1];
    std::ifstream fin(file, std::ifstream::binary | std::ifstream::ate);
    if (std::size_t(fin.tellg()) < buffer_size) {
        std::cout << "file must be at least " << buffer_size << " bytes" << std::endl;
        return 0;
    }

    mmap_read_test(file);
    direct_read_test(file);
    fread_test(file);
    fin_read_test(file);
    vrfs_read_test();
}
