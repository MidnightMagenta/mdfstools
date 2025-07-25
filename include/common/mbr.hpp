#ifndef MDFS_MBR_H
#define MDFS_MBR_H

#include <cstdint>
#include <fstream>

namespace mdfs::mbr {
struct PartitionRecord {
	uint8_t bootIndicator;
	uint8_t startingCHS[3];
	uint8_t OSType;
	uint8_t endingCHS[3];
	uint32_t startingLBA;
	uint32_t sizeInLBA;
} __attribute__((packed));

static_assert(sizeof(PartitionRecord) == 16);

struct MBR {
	uint8_t bootCode[424];
	uint8_t pad[16] = {0};
	uint32_t RDiskSignature;
	uint16_t pad_unknown = 0;
	PartitionRecord partitionRecords[4];
	uint8_t signature[2] = {0x55, 0xAA};
} __attribute__((packed));

static_assert(sizeof(MBR) == 512);

void read_mbr(std::ifstream &file, MBR *mbr);
void write_mbr(std::ofstream &file, const MBR *mbr);
}// namespace mdfs::mbr

#endif