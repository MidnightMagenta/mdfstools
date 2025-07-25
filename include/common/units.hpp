#ifndef MDFS_UNITS_H
#define MDFS_UNITS_H

#include <cstddef>
#include <cstdint>

namespace mdfs {
inline constexpr size_t addr_to_lba(uintptr_t addr, size_t sectorSize = 512) { return addr / sectorSize; }
inline constexpr uintptr_t lba_to_addr(size_t lba, size_t sectorSize = 512) { return lba * sectorSize; }

namespace units {
constexpr uint64_t b = 1;
constexpr uint64_t kb = 1024;
constexpr uint64_t mb = 1048576;
constexpr uint64_t gb = 1073741824;
constexpr uint64_t tb = 1099511627776;
constexpr uint64_t pb = 1125899906842624;
}// namespace units
}// namespace mdfs

#endif