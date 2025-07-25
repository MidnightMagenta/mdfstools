#include <common/guid.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>

void gen_random_UUIDv4(GUID *uuid) {
	std::ifstream rnd("/dev/urandom", std::ios::binary);
	if (!rnd.is_open()) { return; }
	rnd.read((char *) uuid, sizeof(GUID));
	rnd.close();

	uuid->d3 = (uuid->d3 & UUID_VERSION_MASK) | UUIDv4;
	uuid->d4[0] = (uuid->d4[0] & UUID_VARIANT_MASK) | UUID_RFC4122;
}

void print_uuid(const GUID &uuid) {
	std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<uint32_t>(uuid.d1) << "-"
			  << static_cast<uint16_t>(uuid.d2) << "-" << static_cast<uint16_t>(uuid.d3) << "-"
			  << +static_cast<uint8_t>(uuid.d4[0]) << +static_cast<uint8_t>(uuid.d4[1]) << "-"
			  << +static_cast<uint8_t>(uuid.d4[2]) << +static_cast<uint8_t>(uuid.d4[3])
			  << +static_cast<uint8_t>(uuid.d4[4]) << +static_cast<uint8_t>(uuid.d4[5])
			  << +static_cast<uint8_t>(uuid.d4[6]) << +static_cast<uint8_t>(uuid.d4[7]) << "\n";
}