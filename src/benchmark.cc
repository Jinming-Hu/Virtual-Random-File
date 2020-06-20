#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#else
#include "unistd.h"
#include <sys/mman.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <algorithm>

#include "file_ops.h"

constexpr std::size_t buffer_size = 4 * 1024 * 1024 * 1024ULL;
constexpr std::size_t page_size = 4096;

void vrfs_read_test() {
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
#ifdef _WIN32
    // On Winodws, std::ifstream can be really slow.
    std::size_t fin_buffer_size = 1 * 1024 * 1024;
#else
    std::size_t fin_buffer_size = buffer_size;
#endif
    std::ifstream fin(file, std::ifstream::binary);
    std::string buffer;
    buffer.resize(fin_buffer_size);
    auto begin = std::chrono::steady_clock::now();
    fin.read(&buffer[0], buffer.size());
    auto end = std::chrono::steady_clock::now();
    if (static_cast<std::size_t>(fin.gcount()) != buffer.size()) std::abort();
    auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    if (buffer.size() / 1024 / 1024 / 1024 != 0)
        std::cout << "fin_read: " << double(buffer.size() / 1024 / 1024 / 1024) / time_ms * 1000 << "GB/s" << std::endl;
    else
        std::cout << "fin_read: " << double(buffer.size() / 1024 / 1024) / time_ms * 1000 << "MB/s" << std::endl;
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

#ifdef _WIN32
void direct_read_test(const std::string& file)
{
    HANDLE fd = CreateFileA(file.data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (fd == INVALID_HANDLE_VALUE) std::abort();
    std::string buffer;
    buffer.resize(buffer_size);
    auto begin = std::chrono::steady_clock::now();
    std::size_t total_bytes_read = 0;
    while (total_bytes_read < buffer.size())
    {
        DWORD bytes_to_read = static_cast<DWORD>(std::min(buffer.size() - total_bytes_read, static_cast<std::size_t>(std::numeric_limits<DWORD>::max())));
        constexpr DWORD chunk_size = 4 * 1024 * 1024;
        bytes_to_read = bytes_to_read / chunk_size * chunk_size;
        DWORD max_size_to_read = 1 * 1024 * 1024 * 1024ULL;
        bytes_to_read = std::min(bytes_to_read, max_size_to_read);
        DWORD bytes_read;
        BOOL ret = ReadFile(fd, &buffer[0] + total_bytes_read, bytes_to_read, &bytes_read, NULL);
        if (!ret) std::abort();
        if (bytes_read != bytes_to_read) std::abort();
        total_bytes_read += bytes_read;
    }
    auto end = std::chrono::steady_clock::now();
    auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "direct_read_test: " << double(buffer_size / 1024 / 1024 / 1024) / time_ms * 1000 << "GB/s" << std::endl;
    CloseHandle(fd);
}

void mmap_read_test(const std::string& file)
{
    HANDLE fd = CreateFile(file.data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (fd == INVALID_HANDLE_VALUE) std::abort();
    HANDLE map_object = CreateFileMapping(fd, NULL, PAGE_READONLY, 0, 0, NULL);
    if (map_object == NULL) std::abort();
    LARGE_INTEGER map_size;
    map_size.QuadPart = buffer_size;
    LPVOID addr = MapViewOfFile(map_object, FILE_MAP_READ, map_size.HighPart, map_size.LowPart, buffer_size);
    if (addr == NULL) std::abort();
    auto begin = std::chrono::steady_clock::now();
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
    UnmapViewOfFile(addr);
    CloseHandle(map_object);
    CloseHandle(fd);
}

#else
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
#endif

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
