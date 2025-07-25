#ifndef MDFS_PART_INIT_TABLE_H
#define MDFS_PART_INIT_TABLE_H

#include <common/result.hpp>
#include <cstdint>
#include <string>
#include <common/guid.hpp>

namespace mdfs{
	enum class PartType { GPT, MBR };

	struct RunInfo{
		std::string inFile;
		PartType type = PartType::GPT;
		size_t sectorSize = 512;
		size_t partitionEntryCount = 128;
		GUID disk_guid = UUID_NULL;

		bool dryRun = false;
	};

	Result init_table(int argc, char** argv);
}

#endif