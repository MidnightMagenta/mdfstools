#ifndef MDFS_RUN_INFO_H
#define MDFS_RUN_INFO_H

#include <common/djb2.hpp>
#include <cstdint>
#include <utility>
#include <vector>

namespace mdfs {
struct make_partition_info {
	/* data */
};

enum Subcommand {
	NONE,
	INIT_TABLE,
};

std::vector<std::pair<uint64_t, Subcommand>> subcmds = {{djb2("init-table"), INIT_TABLE}};

}// namespace mdfs

#endif