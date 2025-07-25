#include <common/align.hpp>
#include <common/crc32.hpp>
#include <common/djb2.hpp>
#include <common/gpt.hpp>
#include <common/mbr.hpp>
#include <common/units.hpp>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <part/init_table.hpp>
#include <regex>
#include <strings.h>
#include <vector>

void print_help() {}

static mdfs::Result get_run_info(int argc, char **argv, mdfs::RunInfo *runInfo) {
	std::regex cmdRegex(R"(^--([a-zA-Z0-9_-]+)(?:=(.+))?)");
	std::vector<std::cmatch> matches;

	for (int i = 0; i < argc; i++) {
		std::cmatch match;
		if (!std::regex_match(argv[i], match, cmdRegex)) {
			std::cerr << "Unknown parameter: " << argv[i] << "\n";
			return mdfs::Result::FAILURE;
		}
		matches.push_back(match);
	}

	for (const auto &match : matches) {
		std::string fullMatch = match[0].str();
		std::string flag = match[1].str();
		std::string argument = match[2].str();

		switch (mdfs::djb2(flag.c_str())) {
			case mdfs::djb2("img"):
				if (!match[2].matched) {
					std::cerr << "Invalid usage of parameter --img. Usage: --img=[path]\n";
					return mdfs::Result::FAILURE;
				}
				if (!std::filesystem::exists(argument) || !std::filesystem::is_regular_file(argument)) {
					std::cerr << "Parameter is not a path to a file: " << argument << "\n";
					return mdfs::Result::FAILURE;
				}
				runInfo->inFile = argument;
				break;
			case mdfs::djb2("sector-size"):
				if (!match[2].matched) {
					std::cerr << "Invalid usage of parameter --sector-size. Usage: --sector-size=[size in bytes]\n";
					return mdfs::Result::FAILURE;
				}
				if (std::any_of(argument.begin(), argument.end(), [](char c) { return !std::isdigit(c); })) {
					std::cerr << "Value of parameter --sector-size is not a numeric literal\n";
					return mdfs::Result::FAILURE;
				}
				runInfo->sectorSize = std::stoi(argument);
				break;
			case mdfs::djb2("type"):
				if (!match[2].matched) {
					std::cerr << "Invalid usage of parameter --type. Usage: --type=[type]\n";
					return mdfs::Result::FAILURE;
				}
				if (strcasecmp(argument.c_str(), "gpt") == 0) {
					runInfo->type = mdfs::PartType::GPT;
				} else if (strcasecmp(argument.c_str(), "mbr") == 0) {
					runInfo->type = mdfs::PartType::MBR;
				} else {
					std::cerr << "Invalid type for parameter --type. Valid types are GPT and MBR\n";
					return mdfs::Result::FAILURE;
				}
				break;
			case mdfs::djb2("part-entry-count"):
				if (!match[2].matched) {
					std::cerr << "Invalid usage of parameter --type. Usage: --type=[type]\n";
					return mdfs::Result::FAILURE;
				}
				if (std::any_of(argument.begin(), argument.end(), [](char c) { return !std::isdigit(c); })) {
					std::cerr << "Value of parameter --sector-size is not a numeric literal\n";
					return mdfs::Result::FAILURE;
				}
				runInfo->partitionEntryCount = std::stoi(argument);
				break;
			case mdfs::djb2("disk-guid"):
				if (!match[2].matched) {
					std::cerr << "Invalid usage of parameter --type. Usage: --type=[type]\n";
					return mdfs::Result::FAILURE;
				}
				GUID disk_guid;
				if (!get_uuid_from_string(argument, &disk_guid)) {
					std::cerr << "Could not parse UUID: " << argument << "\n";
				}
				runInfo->disk_guid = disk_guid;
				break;
			case mdfs::djb2("dry-run"):
				runInfo->dryRun = true;
				break;
			default:
				std::cerr << "Invalid parameter: " << fullMatch << "\n";
				return mdfs::Result::FAILURE;
		}
	}

	return mdfs::Result::SUCCESS;
}

void print_dry_run(const mdfs::mbr::MBR &protectiveMBR, const mdfs::HeaderGPT &gptPrimaryHeader,
				   const mdfs::HeaderGPT &gptBackupHeader, size_t sectorSize) {
	std::cout << "Will write:\n";
	std::cout << "Protective MBR: \n";

	std::ios_base::fmtflags f(std::cout.flags());
	for (size_t i = 0; i < sizeof(mdfs::mbr::MBR); i++) {
		if ((i % 16) == 0) { std::cout << "\n"; }
		std::cout << std::hex << std::setfill('0') << std::setw(2)
				  << +uint8_t(reinterpret_cast<const char *>(&protectiveMBR)[i]) << " ";
	}
	std::cout.flags(f);
	std::cout << "\n\nGPT primary header at offset: " << gptPrimaryHeader.myLBA << "\n";
	for (size_t i = 0; i < sizeof(mdfs::HeaderGPT); i++) {
		if ((i % 16) == 0) { std::cout << "\n"; }
		std::cout << std::hex << std::setfill('0') << std::setw(2)
				  << +uint8_t(reinterpret_cast<const char *>(&gptPrimaryHeader)[i]) << " ";
	}
	std::cout.flags(f);
	std::cout << "\n\nGPT backup header at offset: " << gptBackupHeader.myLBA << "\n";
	for (size_t i = 0; i < sizeof(mdfs::HeaderGPT); i++) {
		if ((i % 16) == 0) { std::cout << "\n"; }
		std::cout << std::hex << std::setfill('0') << std::setw(2)
				  << +uint8_t(reinterpret_cast<const char *>(&gptBackupHeader)[i]) << " ";
	}
	std::cout.flags(f);
	std::cout << "\n";
}

static mdfs::Result make_partition_table(const mdfs::RunInfo &info) {
	// if GUID not specified, generate a random one
	GUID discGuid = UUID_NULL;
	if (memcmp(&info.disk_guid, &discGuid, sizeof(GUID)) == 0) {
		gen_random_UUIDv4(&discGuid);
	} else {
		discGuid = info.disk_guid;
	}

	// get the size of the disk, and the size of the full GPT (protective MBR + GPT header + partition entry array)
	std::fstream disk(info.inFile, std::ios::in | std::ios::out | std::ios::binary);
	disk.seekp(0, std::ios::end);
	size_t diskSize = mdfs::align_down<size_t>(disk.tellp(), info.sectorSize);

	size_t totalGptSize = mdfs::align_up<size_t>(
			info.sectorSize * 2 + (info.partitionEntryCount * sizeof(mdfs::PartitionEntryGPT)), info.sectorSize);

	//build the structures
	mdfs::mbr::MBR protectiveMBR = mdfs::build_protective_mbr(diskSize, info.sectorSize);
	mdfs::HeaderGPT gptPrimaryHeader = {
			.signature = GPT_SIGNATURE,
			.revision = GPT_REVISION_01,
			.headerSize = sizeof(mdfs::HeaderGPT),
			.headerCRC32 = 0,
			.reserved = {0, 0, 0, 0},
			.myLBA = 1,
			.alternateLBA = mdfs::addr_to_lba(diskSize, info.sectorSize) - 1,
			.firstUsableLBA = mdfs::addr_to_lba(totalGptSize, info.sectorSize),
			.lastUsableLBA = mdfs::addr_to_lba(diskSize - (totalGptSize - info.sectorSize), info.sectorSize),
			.diskGUID = discGuid,
			.partitionEntryLBA = 2,
			.numberOfPartitionEntries = uint32_t(info.partitionEntryCount),
			.sizeOfPartitionEntries = sizeof(mdfs::PartitionEntryGPT),
			.partitionEntryArrayCRC32 = 0};

	mdfs::PartitionEntryGPT *partitionEntryArray = new mdfs::PartitionEntryGPT[info.partitionEntryCount];
	memset(partitionEntryArray, 0x00, info.partitionEntryCount * sizeof(mdfs::PartitionEntryGPT));
	gptPrimaryHeader.partitionEntryArrayCRC32 =
			mdfs::crc32(partitionEntryArray, info.partitionEntryCount * sizeof(mdfs::PartitionEntryGPT));
	delete[] partitionEntryArray;

	gptPrimaryHeader.headerCRC32 = mdfs::crc32(&gptPrimaryHeader, sizeof(mdfs::HeaderGPT));

	mdfs::HeaderGPT gptBackupHeader = gptPrimaryHeader;
	gptBackupHeader.headerCRC32 = 0;
	gptBackupHeader.myLBA = gptPrimaryHeader.alternateLBA;
	gptBackupHeader.alternateLBA = gptPrimaryHeader.myLBA;
	gptBackupHeader.partitionEntryLBA =
			gptBackupHeader.myLBA -
			mdfs::align_up<size_t>(gptPrimaryHeader.sizeOfPartitionEntries * gptPrimaryHeader.numberOfPartitionEntries,
								   info.sectorSize) /
					info.sectorSize;
	gptBackupHeader.headerCRC32 = mdfs::crc32(&gptBackupHeader, sizeof(mdfs::HeaderGPT));

	if (info.dryRun) {
		print_dry_run(protectiveMBR, gptPrimaryHeader, gptBackupHeader, info.sectorSize);
		disk.close();
		return mdfs::Result::SUCCESS;
	}

	//clear the space for the entries
	disk.seekp(0);
	std::vector<char> zeros(diskSize, 0x00);
	disk.write((char *) zeros.data(), zeros.size());

	// write the data structures
	disk.seekp(0);
	disk.write((char *) &protectiveMBR, sizeof(protectiveMBR));

	disk.seekp(mdfs::lba_to_addr(1, info.sectorSize));
	disk.write((char *) &gptPrimaryHeader, sizeof(mdfs::HeaderGPT));

	disk.seekp(mdfs::lba_to_addr(gptPrimaryHeader.alternateLBA, info.sectorSize));
	disk.write((char *) &gptBackupHeader, sizeof(mdfs::HeaderGPT));

	disk.close();
	return mdfs::Result::SUCCESS;
}

mdfs::Result mdfs::init_table(int argc, char **argv) {
	RunInfo info;
	if (get_run_info(argc, argv, &info) != mdfs::Result::SUCCESS) { return mdfs::Result::FAILURE; }

	std::cout << "Sector size " << info.sectorSize << "\n";

	return make_partition_table(info);
}