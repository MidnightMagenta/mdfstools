#ifndef MDFS_CRC32_H
#define MDFS_CRC32_H

#include <cstddef>
#include <cstdint>

typedef uint32_t crc32_t;

namespace mdfs {
crc32_t crc32(const void *data, size_t length, crc32_t init = 0xFFFFFFFF);
}

#endif