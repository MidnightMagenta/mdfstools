#include <common/mbr.hpp>

void mdfs::mbr::read_mbr(std::ifstream &file, MBR *mbr) {
	file.seekg(std::ios::end);
	if (file.tellg() < sizeof(MBR)) { return; }
	file.seekg(0);
	file.read((char *) mbr, sizeof(MBR));
}

void mdfs::mbr::write_mbr(std::ofstream &file, const MBR *mbr) {
	file.seekp(0);
	file.write((char *) mbr, sizeof(MBR));
}