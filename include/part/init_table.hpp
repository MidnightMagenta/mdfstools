#ifndef MDFS_PART_INIT_TABLE_H
#define MDFS_PART_INIT_TABLE_H

#include <common/guid.hpp>
#include <common/result.hpp>
#include <cstdint>
#include <optional>
#include <string>

namespace mdfs {
enum class PartType { GPT, MBR };

struct RunInfo {
	// common
	std::string inFile;
	std::optional<PartType> type;
	std::optional<size_t> sectorSize;

	// GPT specific
	std::optional<size_t> partitionEntryCount;
	std::optional<GUID> disk_guid;

	// MBR specific
	std::string inBootCodeBin;
	std::optional<uint32_t> diskSignature;

	// flags
	bool dryRun = false;
	bool clearAll = false;
	bool strict = false;
};

Result init_table(int argc, char **argv);
}// namespace mdfs

#endif