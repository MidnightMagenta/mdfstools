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

static const uint8_t prot_mbr_code[46] = {
		0xbe, 0x12, 0x7c, 0xac, 0x3c, 0x00, 0x74, 0x06, 0xb4, 0x0e, 0xcd, 0x10, 0xeb, 0xf5, 0xfa, 0xf4,
		0xeb, 0xfc, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x6e, 0x6f, 0x74, 0x20, 0x61, 0x20,
		0x62, 0x6f, 0x6f, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x20, 0x64, 0x69, 0x73, 0x6b, 0x00,
};

void read_mbr(std::ifstream &file, MBR *mbr);
void write_mbr(std::ofstream &file, const MBR *mbr);
}// namespace mdfs::mbr

#endif