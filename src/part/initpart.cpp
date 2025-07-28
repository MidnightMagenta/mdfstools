#include <common/align.hpp>
#include <common/block_device.hpp>
#include <common/crc32.hpp>
#include <common/djb2.hpp>
#include <common/gpt.hpp>
#include <common/mbr.hpp>
#include <common/units.hpp>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <part/initpart.hpp>
#include <random>
#include <vector>

CLI::App *mdfs::make_initpart_app(mdfs::InitPartInfo &info, CLI::App &app) {
	CLI::App *initpart = app.add_subcommand("initpart", "Creates empty partition tables on the provided disk image");
	// common
	initpart->add_option("-i,--img", info.inFile, "Disk image to modify")->required();
	initpart->add_option("-t,--type", info.type, "Partition table type to create")->default_str("GPT");
	initpart->add_option("-s,--sector_size", info.sectorSize, "Sector size to use")->default_val(512);

	// GPT specific
	initpart->add_option("-c,--part-count", info.partitionEntryCount,
						 "Number of partition entries to reserve space for. This option is only valid for GPT")
			->default_val(128);
	initpart->add_option("-g,--guid", info.disk_guid, "The GUID of the disk")->default_str("Random UUIDv4");

	// MBR specific
	initpart->add_option("-b,--boot-code", info.inBootCodeBin,
						 "Path to a boot code binary. This option is only valid for MBR");
	initpart->add_option("-d,--disk-sig", info.diskSignature, "Disk signature. This option is only valid for MBR")
			->default_str("Random UINT32");

	// flags
	initpart->add_flag("-D,--dry", "If specified, disk image won't be modified");
	initpart->add_flag("-C,--clear", "If specified, the full disk image will be zeroed. Otherwise, only the "
									 "sections needed to write the partition tables will be zeroed");
	initpart->add_flag("-S,--strict", "If specified, invalid flags will cause a failure. Otherwise they'll be ignored");

	return initpart;
}

int mdfs::do_initpart(mdfs::InitPartInfo &info, const CLI::App *app) {
	mdfs::InitpartRunInfo runInfo;

	if (app->count("--strict")) { runInfo.strict = true; }

	// common
	if (std::filesystem::exists(info.inFile) && std::filesystem::is_regular_file(info.inFile)) {
		runInfo.inFile = info.inFile;
	} else {
		std::cout << "Specified disk image doesn't exist, or isn't a file.\n";
		return EXIT_FAILURE;
	}

	if (strcasecmp(info.type.c_str(), "gpt") == 0) {
		runInfo.type = mdfs::PartType::GPT;
	} else if (strcasecmp(info.type.c_str(), "mbr") == 0) {
		runInfo.type = mdfs::PartType::MBR;
	} else {
		std::cout << "Invalid partition type\n";
		return EXIT_FAILURE;
	}

	runInfo.sectorSize = info.sectorSize;

	// GPT specific
	if (runInfo.type == mdfs::PartType::GPT) {
		runInfo.partitionEntryCount = info.partitionEntryCount;
		if (info.disk_guid.empty()) {
			gen_random_UUIDv4(&runInfo.disk_guid);
		} else {
			if (!get_uuid_from_string(info.disk_guid, &runInfo.disk_guid)) {
				std::cout << "Could not parse GUID: " << info.disk_guid << "\n";
			}
		}

		if (runInfo.strict) {
			if (app->count("--disk-sig")) {
				std::cout << "Invalid option --disk-sig for MBR partition scheme\n";
				return EXIT_FAILURE;
			}
			if (app->count("--boot-code")) {
				std::cout << "Invalid option --boot-code for MBR partition scheme\n";
				return EXIT_FAILURE;
			}
		}
	} else if (runInfo.type == mdfs::PartType::MBR) {
		runInfo.inBootCodeBin = info.inBootCodeBin;
		if (app->count("--disk-sig")) {
			runInfo.diskSignature = info.diskSignature;
		} else {
			std::random_device rd;
			runInfo.diskSignature = rd();
		}

		if (runInfo.strict) {
			if (app->count("--part-count")) {
				std::cout << "Invalid option --part-count for MBR partition scheme\n";
				return EXIT_FAILURE;
			}
			if (app->count("--disk-guid")) {
				std::cout << "Invalid option --disk-guid for MBR partition scheme\n";
				return EXIT_FAILURE;
			}
		}
	} else {
		std::cout << "Invalid partition type\n";
		return EXIT_FAILURE;
	}

	// flags
	if (app->count("--dry")) { runInfo.dryRun = true; }
	if (app->count("--clear")) { runInfo.clearAll = true; }

	if (mdfs::make_partition_table(runInfo) != mdfs::Result::SUCCESS) { return EXIT_FAILURE; }
	return EXIT_SUCCESS;
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

static mdfs::Result make_gpt_partition_table(const mdfs::InitpartRunInfo &info) {
	// if GUID not specified or NULL, generate a random one
	GUID diskGuid = info.disk_guid;

	std::cout << "Writing GPT partition table...\n"
			  << std::left << std::setw(20) << "Disk image: " << info.inFile << "\n"
			  << std::left << std::setw(20) << "Entry count: " << info.partitionEntryCount << "\n"
			  << std::left << std::setw(20) << "Sector size: " << info.sectorSize << "\n"
			  << std::left << std::setw(20) << "Clear image: " << (info.clearAll ? "true" : "false") << "\n"
			  << std::left << std::setw(20) << "Disk GUID: " << "";
	print_uuid(diskGuid);

	mdfs::BlockDevice disk(info.inFile, info.sectorSize);

	size_t totalGptSize = mdfs::align_up<size_t>(
			info.sectorSize * 2 + (info.partitionEntryCount * sizeof(mdfs::PartitionEntryGPT)), info.sectorSize);

	//build the structures
	mdfs::mbr::MBR protectiveMBR = mdfs::build_protective_mbr(disk.size_b(), info.sectorSize);
	mdfs::HeaderGPT gptPrimaryHeader = {.signature = GPT_SIGNATURE,
										.revision = GPT_REVISION_01,
										.headerSize = sizeof(mdfs::HeaderGPT),
										.headerCRC32 = 0,
										.reserved = {0, 0, 0, 0},
										.myLBA = 1,
										.alternateLBA = disk.size_lba() - 1,
										.firstUsableLBA = mdfs::addr_to_lba(totalGptSize, info.sectorSize),
										.lastUsableLBA = disk.size_lba() - totalGptSize / info.sectorSize,
										.diskGUID = diskGuid,
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
	gptBackupHeader.partitionEntryLBA = gptPrimaryHeader.lastUsableLBA + 1;
	gptBackupHeader.headerCRC32 = mdfs::crc32(&gptBackupHeader, sizeof(mdfs::HeaderGPT));

	if (info.dryRun) {
		print_dry_run(protectiveMBR, gptPrimaryHeader, gptBackupHeader, info.sectorSize);
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

static mdfs::Result make_mbr_partition_table(const mdfs::InitpartRunInfo &info) {
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
	mbr.RDiskSignature = info.diskSignature;
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
	std::cout << std::left << std::setw(20) << std::hex << "Disk signature:" << "0x" << info.diskSignature << std::dec
			  << "\n";

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

mdfs::Result mdfs::make_partition_table(const mdfs::InitpartRunInfo &info) {
	switch (info.type) {
		case mdfs::PartType::GPT:
			return make_gpt_partition_table(info);
		case mdfs::PartType::MBR:
			return make_mbr_partition_table(info);
		default:
			return mdfs::Result::FAILURE;
	}
}