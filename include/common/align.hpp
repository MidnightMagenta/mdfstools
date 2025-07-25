#ifndef MDFS_ALIGN_HPP
#define MDFS_ALIGN_HPP

namespace mdfs {
template<typename T>
inline constexpr T align_up(T val, T alignment) {
	return (T(val) + (T(alignment) - 1)) & (~(T(alignment) - 1));
}

template<typename T>
inline constexpr T align_down(T val, T alignment) {
	return T(val) & ~(T(alignment) - 1);
}
}// namespace mdfs

#endif