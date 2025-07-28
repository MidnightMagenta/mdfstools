#ifndef MDFS_PART_INIT_TABLE_H
#define MDFS_PART_INIT_TABLE_H

#include <common/CLI11.hpp>
#include <common/guid.hpp>
#include <common/result.hpp>
#include <cstdint>
#include <string>

namespace mdfs {
enum class PartType { GPT, MBR };

struct InitPartInfo {
	// common
	std::string inFile = "none";
	std::string type = "GPT";
	size_t sectorSize = 512;

	// GPT specific
	size_t partitionEntryCount = 128;
	std::string disk_guid;

	// MBR specific
	std::string inBootCodeBin;
	uint32_t diskSignature;
};

struct InitpartRunInfo {
	// common
	std::string inFile;
	PartType type;
	size_t sectorSize;

	// GPT specific
	size_t partitionEntryCount;
	GUID disk_guid;

	// MBR specific
	std::string inBootCodeBin;
	uint32_t diskSignature;

	// flags
	bool dryRun = false;
	bool clearAll = false;
	bool strict = false;
};
CLI::App *make_initpart_app(mdfs::InitPartInfo &info, CLI::App &app);
int do_initpart(mdfs::InitPartInfo &info, const CLI::App *app);
Result make_partition_table(const mdfs::InitpartRunInfo &info);
}// namespace mdfs

#endif