#include <common/align.hpp>
#include <common/block_device.hpp>
#include <common/crc32.hpp>
#include <common/djb2.hpp>
#include <common/gpt.hpp>
#include <common/mbr.hpp>
#include <common/units.hpp>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <part/init_table.hpp>
#include <random>
#include <regex>
#include <strings.h>
#include <vector>

static void print_help() {
	std::cout << "\033[1mSYNOPSIS\033[0m\n";
	std::cout << std::left << std::setw(5) << "" << "mdfst init-table [options]\n\n";

	std::cout << "\033[1mDESCRIPTION\033[0m\n";
	std::cout << std::left << std::setw(5) << ""
			  << "The init-table option creates empty page tables of MBR or GPT types on the provided disk image.\n\n";

	std::cout << "\033[1mOPTIONS\033[0m\n";

	std::cout << std::left << std::setw(5) << ""
			  << "--img=[path]\n";
	std::cout << std::left << std::setw(10) << ""
			  << "Used to specify the path to the disk image onto which the page tables will be written.\n";
	std::cout << std::left << std::setw(10) << ""
			  << "This option must be specified. The path must be non empty, and lead to a regular file.\n\n";

	std::cout << std::left << std::setw(5) << ""
			  << "--sector-size=[size in bytes]\n";
	std::cout
			<< std::left << std::setw(10) << ""
			<< "Used to specify the sector size the disk image will use. If not specified, sector size will default\n";
	std::cout << std::left << std::setw(10) << ""
			  << "to 512 bytes.\n\n";

	std::cout << std::left << std::setw(5) << ""
			  << "--type=[type]\n";
	std::cout << std::left << std::setw(10) << ""
			  << "Used to specify the type of partition tables that will be created. Valid options are \"MBR\" and\n";
	std::cout << std::left << std::setw(10) << ""
			  << "\"GPT\". If not specified, the type will default to GPT\n\n";

	std::cout << std::left << std::setw(5) << ""
			  << "--part-entry-count=[count]\n";
	std::cout << std::left << std::setw(10) << ""
			  << "Used to specify the number of partition entries for which space will be reserved in the image.\n";
	std::cout << std::left << std::setw(10) << ""
			  << "If this option is not specified, the number of entries will default to 128. This option is valid\n";
	std::cout << std::left << std::setw(10) << ""
			  << "only if partition table type is GPT\n\n";


	std::cout << std::left << std::setw(5) << ""
			  << "--disk-guid=[UUIDv4]\n";
	std::cout << std::left << std::setw(10) << ""
			  << "Used to specify the GUID of the created disk image. If not specified or the passed UUID is\n";
	std::cout << std::left << std::setw(10) << ""
			  << "00000000-0000-0000-0000-000000000000 the disk's GUID will be randomly generated. This option\n";
	std::cout << std::left << std::setw(10) << ""
			  << "is valid only if partition table type is GPT\n\n";

	std::cout << std::left << std::setw(5) << ""
			  << "--boot-code=[path]\n";
	std::cout << std::left << std::setw(10) << ""
			  << "Used to specify the path to a binary file containing boot code to be inserted into the MBR\n";
	std::cout << std::left << std::setw(10) << ""
			  << "partition table. The binary file may be no larger than 424 bytes. This option is valid only\n";
	std::cout << std::left << std::setw(10) << ""
			  << "if partition table type is MBR\n\n";

	std::cout << std::left << std::setw(5) << ""
			  << "--disk-signature=[signature]\n";
	std::cout << std::left << std::setw(10) << ""
			  << "Used to specify disk signature. This option is valid only if partition table type is MBR.\n\n";

	std::cout << std::left << std::setw(5) << ""
			  << "--clear-image\n";
	std::cout << std::left << std::setw(10) << ""
			  << "If this flag is specified, the image will be zeroed in it's entirety. Otherwise only the area\n";
	std::cout << std::left << std::setw(10) << ""
			  << "used to write the partition tables will be zeroed.\n\n";

	std::cout << std::left << std::setw(5) << ""
			  << "--dry-run\n";
	std::cout << std::left << std::setw(10) << ""
			  << "If this flag is specified, the provided image will not be modified, and the program will print\n";
	std::cout << std::left << std::setw(10) << ""
			  << "the binary data it would write to the image to the terminal.\n\n";

	std::cout << std::left << std::setw(5) << ""
			  << "--strict\n";
	std::cout << std::left << std::setw(10) << ""
			  << "If this flag is specified, if any invalid flag is passed, the program will abort. Otherwise the\n";
	std::cout << std::left << std::setw(10) << ""
			  << "invalid flag will be ignored.\n\n";

	std::cout << std::left << std::setw(5) << ""
			  << "--help\n";
	std::cout << std::left << std::setw(10) << ""
			  << "Prints this message\n\n";

	std::cout << "\033[1mEXAMPLE USAGE\033[0m\n";
	std::cout << std::left << std::setw(5) << ""
			  << "mdfst init-image --img=path/to/disk.img --sector-size=4096 --type=GPT --part-entry-count=256\n";
	std::cout << std::left << std::setw(22) << ""
			  << "--disk-guid=c28c3ae5-41ba-4691-b566-f9ce96b5191e --clear-image --dry-run\n";
}

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
				if (!runInfo->inFile.empty()) {
					std::cerr << "Can't redefine --img\n";
					return mdfs::Result::FAILURE;
				}
				if (!match[2].matched) {
					std::cerr << "Invalid usage of parameter --img. Usage: --img=[path]\n";
					return mdfs::Result::FAILURE;
				}
				if (argument.empty() || !std::filesystem::exists(argument) ||
					!std::filesystem::is_regular_file(argument)) {
					std::cerr << "Parameter is not a path to a file: " << argument << "\n";
					return mdfs::Result::FAILURE;
				}
				runInfo->inFile = argument;
				break;
			case mdfs::djb2("sector-size"):
				if (runInfo->sectorSize.has_value()) {
					std::cerr << "Can't redefine --sector-size\n";
					return mdfs::Result::FAILURE;
				}
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
				if (runInfo->type.has_value()) {
					std::cerr << "Can't redefine --type\n";
					return mdfs::Result::FAILURE;
				}
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
				if (runInfo->partitionEntryCount.has_value()) {
					std::cerr << "Can't redefine --part-entry-count\n";
					return mdfs::Result::FAILURE;
				}
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
				if (runInfo->disk_guid.has_value()) {
					std::cerr << "Can't redefine --disk-guid\n";
					return mdfs::Result::FAILURE;
				}
				if (!match[2].matched) {
					std::cerr << "Invalid usage of parameter --disk-guid. Usage: --disk-guid=[UUIDv4]\n";
					return mdfs::Result::FAILURE;
				}
				GUID disk_guid;
				if (!get_uuid_from_string(argument, &disk_guid)) {
					std::cerr << "Could not parse UUID: " << argument << "\n";
				}
				runInfo->disk_guid = disk_guid;
				break;
			case mdfs::djb2("boot-code"):
				if (!runInfo->inBootCodeBin.empty()) {
					std::cerr << "Can't redefine --boot-code\n";
					return mdfs::Result::FAILURE;
				}
				if (!match[2].matched) {
					std::cerr << "Invalid usage of parameter --boot-code. Usage: --boot-code=[path]\n";
					return mdfs::Result::FAILURE;
				}
				if (argument.empty() || !std::filesystem::exists(argument) ||
					!std::filesystem::is_regular_file(argument)) {
					std::cerr << "Parameter is not a path to a file: " << argument << "\n";
					return mdfs::Result::FAILURE;
				}
				runInfo->inBootCodeBin = argument;
				break;
			case mdfs::djb2("disk-signature"):
				if (!runInfo->inBootCodeBin.empty()) {
					std::cerr << "Can't redefine --boot-code\n";
					return mdfs::Result::FAILURE;
				}
				if (!match[2].matched) {
					std::cerr << "Invalid usage of parameter --boot-code. Usage: --boot-code=[path]\n";
					return mdfs::Result::FAILURE;
				}
				if (std::any_of(argument.begin(), argument.end(), [](char c) { return !std::isdigit(c); })) {
					std::cerr << "Value of parameter --boot-code is not a numeric literal\n";
					return mdfs::Result::FAILURE;
				}
				runInfo->diskSignature = std::stoi(argument);
				break;
			case mdfs::djb2("dry-run"):
				runInfo->dryRun = true;
				break;
			case mdfs::djb2("clear-image"):
				runInfo->clearAll = true;
				break;
			case mdfs::djb2("strict"):
				runInfo->strict = true;
				break;
			case mdfs::djb2("help"):
				print_help();
				return mdfs::Result::NO_WORK;
			default:
				std::cerr << "Invalid parameter: " << fullMatch << "\n";
				return mdfs::Result::FAILURE;
		}
	}

	// verify the passed flags
	if (runInfo->inFile.empty()) {
		std::cout << "--img paremeter not specified. Use --img=[path]\n";
		return mdfs::Result::FAILURE;
	}
	if (!runInfo->sectorSize.has_value()) { runInfo->sectorSize = 512; }
	if (!runInfo->type.has_value()) { runInfo->type = mdfs::PartType::GPT; }

	if (runInfo->type == mdfs::PartType::MBR) {
		if (runInfo->disk_guid.has_value()) {
			std::cout << "--disk-guid is invalid for MBR partition. ";
			if (runInfo->strict) {
				std::cout << "Aborting\n";
				return mdfs::Result::FAILURE;
			}
			std::cout << "Ignoring\n";
		}

		if (runInfo->partitionEntryCount.has_value()) {
			std::cout << "--part-entry-count is invalid for MBR partition. ";
			if (runInfo->strict) {
				std::cout << "Aborting\n";
				return mdfs::Result::FAILURE;
			}
			std::cout << "Ignoring\n";
		}

		if (!runInfo->diskSignature.has_value()) {
			std::random_device rd;
			runInfo->diskSignature = rd();
		}
	} else if (runInfo->type == mdfs::PartType::GPT) {
		if (!runInfo->inBootCodeBin.empty()) {
			std::cout << "--boot-code is invalid for GPT partition. ";
			if (runInfo->strict) {
				std::cout << "Aborting\n";
				return mdfs::Result::FAILURE;
			}
			std::cout << "Ignoring\n";
		}

		if (runInfo->diskSignature.has_value()) {
			std::cout << "--disk-signature is invalid for GPT partition. ";
			if (runInfo->strict) {
				std::cout << "Aborting\n";
				return mdfs::Result::FAILURE;
			}
			std::cout << "Ignoring\n";
		}

		if (!runInfo->partitionEntryCount.has_value()) {
			runInfo->partitionEntryCount = 128;
		} else if (runInfo->partitionEntryCount.value() < 128) {
			std::cout << "GPT must have at least 128 partition entries. ";
			if (runInfo->strict) {
				std::cout << "Aborting\n";
				return mdfs::Result::FAILURE;
			}
			std::cout << "Settinng partition entry count to 128\n";
			runInfo->partitionEntryCount = 128;
		}

		if (!runInfo->disk_guid.has_value()) {
			runInfo->disk_guid = UUID_NULL;
			gen_random_UUIDv4(&runInfo->disk_guid.value());
		}
	} else {
		std::cout << "Invalid partition type\n";
		return mdfs::Result::FAILURE;
	}

	return mdfs::Result::SUCCESS;
}

static void print_dry_run(const mdfs::mbr::MBR &protectiveMBR, const mdfs::HeaderGPT &gptPrimaryHeader,
						  const mdfs::HeaderGPT &gptBackupHeader, size_t sectorSize) {
	std::cout << "\n\033[1mDry run info\033[0m\n\n";
	std::cout << "Protective MBR:";

	std::ios_base::fmtflags f(std::cout.flags());
	for (size_t i = 0; i < sizeof(mdfs::mbr::MBR); i++) {
		if ((i % 16) == 0) { std::cout << "\n"; }
		std::cout << std::hex << std::setfill('0') << std::setw(2)
				  << +uint8_t(reinterpret_cast<const char *>(&protectiveMBR)[i]) << " ";
	}
	std::cout.flags(f);
	std::cout << "\n\nGPT primary header at offset: " << gptPrimaryHeader.myLBA;
	for (size_t i = 0; i < sizeof(mdfs::HeaderGPT); i++) {
		if ((i % 16) == 0) { std::cout << "\n"; }
		std::cout << std::hex << std::setfill('0') << std::setw(2)
				  << +uint8_t(reinterpret_cast<const char *>(&gptPrimaryHeader)[i]) << " ";
	}
	std::cout.flags(f);
	std::cout << "\n\nGPT backup header at offset: " << gptBackupHeader.myLBA;
	for (size_t i = 0; i < sizeof(mdfs::HeaderGPT); i++) {
		if ((i % 16) == 0) { std::cout << "\n"; }
		std::cout << std::hex << std::setfill('0') << std::setw(2)
				  << +uint8_t(reinterpret_cast<const char *>(&gptBackupHeader)[i]) << " ";
	}
	std::cout.flags(f);
	std::cout << "\n";
}

static mdfs::Result make_gpt_partition_table(const mdfs::RunInfo &info) {
	// if GUID not specified or NULL, generate a random one
	GUID diskGuid;
	if (info.disk_guid.has_value()) {
		diskGuid = info.disk_guid.value();
	} else {
		gen_random_UUIDv4(&diskGuid);
	}

	std::cout << "Writing GPT partition table...\n"
			  << std::left << std::setw(20) << "Disk image: " << info.inFile << "\n"
			  << std::left << std::setw(20) << "Entry count: " << info.partitionEntryCount.value() << "\n"
			  << std::left << std::setw(20) << "Sector size: " << info.sectorSize.value() << "\n"
			  << std::left << std::setw(20) << "Clear image: " << (info.clearAll ? "true" : "false") << "\n"
			  << std::left << std::setw(20) << "Disk GUID: " << "";
	print_uuid(diskGuid);

	mdfs::BlockDevice disk(info.inFile, info.sectorSize.value());

	size_t totalGptSize = mdfs::align_up<size_t>(
			info.sectorSize.value() * 2 + (info.partitionEntryCount.value() * sizeof(mdfs::PartitionEntryGPT)),
			info.sectorSize.value());

	//build the structures
	mdfs::mbr::MBR protectiveMBR = mdfs::build_protective_mbr(disk.size_b(), info.sectorSize.value());
	mdfs::HeaderGPT gptPrimaryHeader = {.signature = GPT_SIGNATURE,
										.revision = GPT_REVISION_01,
										.headerSize = sizeof(mdfs::HeaderGPT),
										.headerCRC32 = 0,
										.reserved = {0, 0, 0, 0},
										.myLBA = 1,
										.alternateLBA = disk.size_lba() - 1,
										.firstUsableLBA = mdfs::addr_to_lba(totalGptSize, info.sectorSize.value()),
										.lastUsableLBA = disk.size_lba() - totalGptSize / info.sectorSize.value(),
										.diskGUID = diskGuid,
										.partitionEntryLBA = 2,
										.numberOfPartitionEntries = uint32_t(info.partitionEntryCount.value()),
										.sizeOfPartitionEntries = sizeof(mdfs::PartitionEntryGPT),
										.partitionEntryArrayCRC32 = 0};

	mdfs::PartitionEntryGPT *partitionEntryArray = new mdfs::PartitionEntryGPT[info.partitionEntryCount.value()];
	memset(partitionEntryArray, 0x00, info.partitionEntryCount.value() * sizeof(mdfs::PartitionEntryGPT));
	gptPrimaryHeader.partitionEntryArrayCRC32 =
			mdfs::crc32(partitionEntryArray, info.partitionEntryCount.value() * sizeof(mdfs::PartitionEntryGPT));
	delete[] partitionEntryArray;

	gptPrimaryHeader.headerCRC32 = mdfs::crc32(&gptPrimaryHeader, sizeof(mdfs::HeaderGPT));

	mdfs::HeaderGPT gptBackupHeader = gptPrimaryHeader;
	gptBackupHeader.headerCRC32 = 0;
	gptBackupHeader.myLBA = gptPrimaryHeader.alternateLBA;
	gptBackupHeader.alternateLBA = gptPrimaryHeader.myLBA;
	gptBackupHeader.partitionEntryLBA = gptPrimaryHeader.lastUsableLBA + 1;
	gptBackupHeader.headerCRC32 = mdfs::crc32(&gptBackupHeader, sizeof(mdfs::HeaderGPT));

	if (info.dryRun) {
		print_dry_run(protectiveMBR, gptPrimaryHeader, gptBackupHeader, info.sectorSize.value());
		disk.close();
		return mdfs::Result::SUCCESS;
	}

	//clear the space for the entries
	std::vector<char> zeros(disk.size_b(), 0x00);
	if (info.clearAll) {
		disk.seekp(0);
		disk.write((char *) zeros.data(), zeros.size());
	} else {
		disk.seekp(0);
		disk.write((char *) zeros.data(), totalGptSize);
		disk.seekp((disk.size_b() - totalGptSize) / disk.block_size());
		disk.write((char *) zeros.data(), totalGptSize);
	}

	// write the data structures
	disk.seekp(0);
	disk.write((char *) &protectiveMBR, sizeof(mdfs::mbr::MBR));
	disk.seekp(1);
	disk.write((char *) &gptPrimaryHeader, sizeof(mdfs::HeaderGPT));
	disk.seekp(gptPrimaryHeader.alternateLBA);
	disk.write((char *) &gptBackupHeader, sizeof(mdfs::HeaderGPT));

	disk.close();
	return mdfs::Result::SUCCESS;
}

static mdfs::Result make_mbr_partition_table(const mdfs::RunInfo &info) {
	mdfs::mbr::MBR mbr;
	memset(&mbr, 0xF4, 424);
	if (info.inBootCodeBin.empty()) {
		memcpy(&mbr, mdfs::mbr::prot_mbr_code, 46);
	} else {
		std::ifstream inBootCode(info.inBootCodeBin, std::ios::binary | std::ios::ate);
		size_t bootCodeSize = inBootCode.tellg();
		if (bootCodeSize > 424) {
			std::cerr << "Boot code can't fit in MBR boot code area.\n";
			return mdfs::Result::FAILURE;
		}
		inBootCode.seekg(0);
		inBootCode.read((char *) &mbr, bootCodeSize);
	}
	mbr.RDiskSignature = info.diskSignature.value();
	memset(&mbr.partitionRecords, 0x00, 4 * sizeof(mdfs::mbr::PartitionRecord));

	if (info.dryRun) {
		std::cout << "Will write:\n";
		std::ios_base::fmtflags f(std::cout.flags());
		for (size_t i = 0; i < sizeof(mdfs::mbr::MBR); i++) {
			if ((i % 16) == 0) { std::cout << "\n"; }
			std::cout << std::hex << std::setfill('0') << std::setw(2)
					  << +uint8_t(reinterpret_cast<const char *>(&mbr)[i]) << " ";
		}
		std::cout.flags(f);
		std::cout << "\n";
		return mdfs::Result::SUCCESS;
	}
	std::cout << "Writing MBR partition table\n";
	std::cout << std::left << std::setw(20)
			  << "Boot code:" << (info.inBootCodeBin.empty() ? "Default" : info.inBootCodeBin) << "\n";
	std::cout << std::left << std::setw(20) << std::hex << "Disk signature:" << "0x" << info.diskSignature.value()
			  << std::dec << "\n";

	// std::fstream disk(info.inFile, std::ios::in | std::ios::out | std::ios::binary);
	// disk.seekp(0, std::ios::end);
	// size_t diskSize = mdfs::align_down<size_t>(disk.tellp(), info.sectorSize.value());

	mdfs::BlockDevice disk(info.inFile);

	std::vector<char> zeros(disk.size_b(), 0x00);
	if (info.clearAll) {
		disk.seekp(0);
		disk.write((char *) zeros.data(), zeros.size());
	}

	disk.seekp(0);
	disk.write((char *) &mbr, sizeof(mdfs::mbr::MBR));
	return mdfs::Result::SUCCESS;
}

static mdfs::Result make_partition_table(const mdfs::RunInfo &info) {
	switch (info.type.value()) {
		case mdfs::PartType::GPT:
			return make_gpt_partition_table(info);
		case mdfs::PartType::MBR:
			return make_mbr_partition_table(info);
		default:
			return mdfs::Result::FAILURE;
	}
}

mdfs::Result mdfs::init_table(int argc, char **argv) {
	RunInfo info;
	mdfs::Result res = get_run_info(argc, argv, &info);
	if (res != mdfs::Result::SUCCESS) {
		if (res == mdfs::Result::NO_WORK) { return mdfs::Result::SUCCESS; }
		return mdfs::Result::FAILURE;
	}
	return make_partition_table(info);
}