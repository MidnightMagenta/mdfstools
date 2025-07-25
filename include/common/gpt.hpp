#ifndef MDFS_GPT_H
#define MDFS_GPT_H

#include <common/crc32.hpp>
#include <common/guid.hpp>
#include <common/mbr.hpp>
#include <cstddef>
#include <cstdint>

#define GPT_UNUSED_PARTITION_ENTRY_GUID {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}
#define GPT_EFI_SYSTEM_PARTITION_GUID {0xC12A7328, 0xF81F, 0x11D2, {0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B}}
#define GPT_PARTITION_CONTAINING_LEGACY_MBR_GUID                                                                       \
	{0x024DEE41, 0x33E7, 0x11D3, {0x9D, 0x69, 0x00, 0x08, 0xC7, 0x81, 0xF3, 0x9F}}

#define GPT_REVISION_01 0x00010000

namespace mdfs {
struct HeaderGPT {
	char signature[8];
	uint32_t revision = GPT_REVISION_01;
	uint32_t headerSize;
	crc32_t headerCRC32;
	uint8_t reserved[4];
	uint64_t myLBA;
	uint64_t alternateLBA;
	uint64_t firstUsableLBA;
	uint64_t lastUsableLBA;
	GUID diskGUID;
	uint64_t partitionEntryLBA;
	uint32_t numberOfPartitionEntries;
	uint32_t sizeOfPartitionEntries;
	crc32_t partitionEntryArrayCRC32;
} __attribute__((packed));
static_assert(sizeof(HeaderGPT) == 92);

struct PartitionEntryGPT {
	GUID partitionTypeGUID;
	GUID uniquePartitionGUID;
	uint64_t startingLBA;
	uint64_t endingLBA;
	uint64_t attributes;
	char16_t partitionName[36];
} __attribute__((packed));
static_assert(sizeof(PartitionEntryGPT) == 128);

mdfs::mbr::MBR build_protective_mbr(size_t diskSize, size_t sectorSize = 512);
}// namespace mdfs

#endif