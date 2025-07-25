#ifndef MDFS_DJB2_H
#define MDFS_DJB2_H

#include <cstdint>

namespace mdfs {
inline constexpr uint64_t djb2(const char *str) {
	uint64_t hash = 5381;

	while (*str) { hash = ((hash << 5) + hash) + *str++; }

	return hash;
}
}// namespace mdfs
#endif