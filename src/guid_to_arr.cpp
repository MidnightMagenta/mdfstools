#include <cstdlib>
#include <iostream>
#include <regex>
#include <string>

#define GUID_REGEX                                                                                                     \
	R"(^([0-9a-fA-F]{8})-([0-9a-fA-F]{4})-([0-9a-fA-F]{4})-([0-9a-fA-F]{2})([0-9a-fA-F]{2})-([0-9a-fA-F]{12})$)"

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "Invalid arguments\n";
		return EXIT_FAILURE;
	}

	std::string in = argv[1];
	std::regex guid_regex(GUID_REGEX);
	std::smatch matches;

	if (!std::regex_match(in, matches, guid_regex) || matches.size() < 7) {
		std::cerr << "Failed to match UUID\n";
		return EXIT_FAILURE;
	}

	std::cout << "{ 0x" << matches[1].str() << ", 0x" << matches[2].str() << ", 0x" << matches[3].str() << ", { 0x"
			  << matches[4].str() << ", 0x" << matches[5].str();

	for (size_t i = 0; i < 12; i += 2) {
		std::string d4 = matches[6].str().substr(i, 2);
		std::cout << ", 0x" << d4;
	}

	std::cout << "}}\n";
	return EXIT_SUCCESS;
}