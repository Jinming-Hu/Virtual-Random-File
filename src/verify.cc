#include "file_ops.h"

#include <cstdlib>
#include <iostream>


void invariance_verify() {
    constexpr std::size_t global_offset = 100ULL * 1024 * 1024 * 1024 * 1024;
    constexpr std::size_t buffer_size = 8 * 1024 * 1024;

    std::string buffer;
    buffer.resize(buffer_size);
    vrfs_read(nullptr, &buffer[0], buffer.size(), global_offset, nullptr);
    std::string buffer2 = buffer;
    for (int i = 0; i < 1000; ++i) {
        int a = rand() % buffer.size();
        int b = rand() % buffer.size();
        if (a > b) std::swap(a, b);
        std::fill(buffer2.begin() + a, buffer2.begin() + b, '\x00');
        vrfs_read(nullptr, &buffer2[a], b - a, a + global_offset, nullptr);
        if (buffer != buffer2) std::abort();
    }
}

void frequency_gap_verify() {
    constexpr std::size_t buffer_size = 256 * 1024 * 1024;
    std::string buffer;
    buffer.resize(buffer_size);
    vrfs_read(nullptr, &buffer[0], buffer.size(), 0, nullptr);
    std::size_t freq[256]{};
    std::size_t last_pos[256]{};
    std::size_t total_dist[256]{};
    std::size_t not_first_appear[256]{};
    for (std::size_t i = 0; i < buffer.size(); ++i) {
        char c = buffer[i];

        ++freq[uint8_t(c)];
        if (not_first_appear[uint8_t(c)])
            total_dist[uint8_t(c)] += i - last_pos[uint8_t(c)];
        not_first_appear[uint8_t(c)] = true;
        last_pos[uint8_t(c)] = i;
    }
    for (std::size_t f : freq) {
        double prob = double(f) / buffer.size();
        if (!(std::abs(prob - 1.0/256) < 1e-6)) std::abort();
    }
    for (int i = 0; i < 256; ++i) {
        double avg_gap = double(total_dist[i]) / freq[i];
        if (!(std::abs(avg_gap - 256) < 0.1)) std::abort();
    }
}

void serial_verify() {
    constexpr std::size_t buffer_size = 8;
    std::string buffer;
    buffer.resize(buffer_size);
    vrfs_read(nullptr, &buffer[0], buffer.size(), 0, nullptr);

    for (std::size_t offset = 4; offset <= 256ULL * 1024 * 1024 * 1024 * 1024; offset *= 2) {
        std::string buffer2;
        buffer2.resize(buffer_size);
        vrfs_read(nullptr, &buffer2[0], buffer2.size(), offset, nullptr);
        if (buffer2 == buffer) std::abort();
    }
}

int main(int /* argc */, char** /* argv */) {
    invariance_verify();
    frequency_gap_verify();
    serial_verify();
}
