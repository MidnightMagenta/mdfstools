#include <common/djb2.hpp>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <part/init_table.hpp>

int main(int argc, char **argv) {
	if (argc < 2) {
		std::cerr << "Invalid argument combination\n";
		return EXIT_FAILURE;
	}
	std::string app = argv[0];
	std::string subcmdString = argv[1];

	switch (mdfs::djb2(subcmdString.data())) {
		case mdfs::djb2("init-table"):
			mdfs::init_table(argc - 2, argv + 2);
			break;
		default:
			std::cerr << "Invalid parameter: " << subcmdString << "\n";
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}