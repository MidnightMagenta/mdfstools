#include <common/crc32.hpp>
#include <common/gpt.hpp>
#include <common/units.hpp>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <part/run_info.hpp>

int main(int argc, char **argv) {
	if (argc < 2) {
		std::cerr << "Invalid argument combination\n";
		return EXIT_FAILURE;
	}
	std::string app = argv[0];
	std::string subcmdString = argv[1];

	uint64_t subcmdHash = mdfs::djb2(subcmdString.data());
	mdfs::Subcommand subcmd = mdfs::Subcommand::NONE;

	for (size_t i = 0; i < mdfs::subcmds.size(); i++) {
		if (mdfs::subcmds[i].first == subcmdHash) {
			subcmd = mdfs::subcmds[i].second;
			break;
		}
	}

	if(subcmd == mdfs::Subcommand::NONE){
		std::cerr << "Invalid parameter: " << subcmdString << "\n";
		return EXIT_FAILURE;
	}

	for (int i = 2; i < argc; i++) {}

	return EXIT_SUCCESS;
}