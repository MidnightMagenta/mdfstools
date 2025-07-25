#include <common/guid.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>

#define GUID_REGEX                                                                                                     \
	R"(^([0-9a-fA-F]{8})-([0-9a-fA-F]{4})-([0-9a-fA-F]{4})-([0-9a-fA-F]{2})([0-9a-fA-F]{2})-([0-9a-fA-F]{12})$)"

void gen_random_UUIDv4(GUID *uuid) {
	std::ifstream rnd("/dev/urandom", std::ios::binary);
	if (!rnd.is_open()) { return; }
	rnd.read((char *) uuid, sizeof(GUID));
	rnd.close();

	uuid->d3 = (uuid->d3 & UUID_VERSION_MASK) | UUIDv4;
	uuid->d4[0] = (uuid->d4[0] & UUID_VARIANT_MASK) | UUID_RFC4122;
}

#include <iomanip>
#include <iostream>

void print_uuid(const GUID &uuid) {
	std::ios_base::fmtflags f(std::cout.flags());

	std::cout << std::hex << std::setfill('0') << std::setw(8) << uuid.d1 << "-" << std::setw(4) << uuid.d2 << "-"
			  << std::setw(4) << uuid.d3 << "-" << std::setw(2) << static_cast<int>(uuid.d4[0]) << std::setw(2)
			  << static_cast<int>(uuid.d4[1]) << "-" << std::setw(2) << static_cast<int>(uuid.d4[2]) << std::setw(2)
			  << static_cast<int>(uuid.d4[3]) << std::setw(2) << static_cast<int>(uuid.d4[4]) << std::setw(2)
			  << static_cast<int>(uuid.d4[5]) << std::setw(2) << static_cast<int>(uuid.d4[6]) << std::setw(2)
			  << static_cast<int>(uuid.d4[7]) << std::dec << "\n";

	std::cout.flags(f);
}


bool get_uuid_from_string(const std::string &str, GUID *uuid) {
	std::regex guid_regex(GUID_REGEX);
	std::smatch matches;

	if (!std::regex_match(str, matches, guid_regex) || matches.size() < 7) {
		std::cerr << "Failed to match UUID\n";
		return false;
	}

	auto parse_hex = [](const std::string &s) {
		uint64_t val = 0;
		std::stringstream ss(s);
		ss >> std::hex >> val;
		return val;
	};

	uuid->d1 = static_cast<uint32_t>(parse_hex(matches[1].str()));
	uuid->d2 = static_cast<uint16_t>(parse_hex(matches[2].str()));
	uuid->d3 = static_cast<uint16_t>(parse_hex(matches[3].str()));
	uuid->d4[0] = static_cast<uint8_t>(parse_hex(matches[4].str()));
	uuid->d4[1] = static_cast<uint8_t>(parse_hex(matches[5].str()));

	const std::string &lastPart = matches[6].str();
	for (int i = 0; i < 6; ++i) {
		std::string byteStr = lastPart.substr(i * 2, 2);
		uuid->d4[i + 2] = static_cast<uint8_t>(parse_hex(byteStr));
	}

	return true;
}