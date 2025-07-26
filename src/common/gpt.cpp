#include <cassert>
#include <common/gpt.hpp>
#include <cstring>

mdfs::mbr::MBR mdfs::build_protective_mbr(size_t diskSize, size_t sectorSize) {
	mdfs::mbr::MBR protectiveMBR;
	memset(&protectiveMBR.bootCode, 0xF4, 424);
	memcpy(&protectiveMBR.bootCode, mdfs::mbr::prot_mbr_code, 46);

	protectiveMBR.partitionRecords[0] = {
			.bootIndicator = 0x00,
			.startingCHS = {0x00, 0x02, 0x00},
			.OSType = 0xEE,
			.endingCHS = {0xFF, 0xFF, 0xFF},
			.startingLBA = 0x00000001,
			.sizeInLBA = ((diskSize / sectorSize) > UINT32_MAX) ? 0xFFFFFFFF : uint32_t(diskSize / sectorSize) - 1};
	protectiveMBR.partitionRecords[1] = {.bootIndicator = 0x00,
										 .startingCHS = {0x00, 0x00, 0x00},
										 .OSType = 0x00,
										 .endingCHS = {0x00, 0x00, 0x00},
										 .startingLBA = 0x00000000,
										 .sizeInLBA = 0x00000000};
	protectiveMBR.partitionRecords[2] = {.bootIndicator = 0x00,
										 .startingCHS = {0x00, 0x00, 0x00},
										 .OSType = 0x00,
										 .endingCHS = {0x00, 0x00, 0x00},
										 .startingLBA = 0x00000000,
										 .sizeInLBA = 0x00000000};
	protectiveMBR.partitionRecords[3] = {.bootIndicator = 0x00,
										 .startingCHS = {0x00, 0x00, 0x00},
										 .OSType = 0x00,
										 .endingCHS = {0x00, 0x00, 0x00},
										 .startingLBA = 0x00000000,
										 .sizeInLBA = 0x00000000};

	assert(protectiveMBR.signature[0] == 0x55 && protectiveMBR.signature[1] == 0xAA);
	return protectiveMBR;
}