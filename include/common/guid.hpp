#ifndef MDFS_GUID_H
#define MDFS_GUID_H

#include <cstdint>
#include <string>

#define UUIDv4 static_cast<uint16_t>(0x4000)
#define UUID_RFC4122 static_cast<uint8_t>(0x80)

#define UUID_VERSION_MASK static_cast<uint16_t>(~0xF000)
#define UUID_VARIANT_MASK static_cast<uint8_t>(~0xC0)

#define UUID_NULL GUID({0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}})

struct GUID {
	uint32_t d1;
	uint16_t d2;
	uint16_t d3;
	uint8_t d4[8];
} __attribute__((packed));

void gen_random_UUIDv4(GUID *uuid);
void print_uuid(const GUID &uuid);
bool get_uuid_from_string(const std::string &str, GUID *uuid);
#endif