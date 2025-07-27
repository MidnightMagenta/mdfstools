#ifndef MDFS_BLOCK_DEVICE_H
#define MDFS_BLOCK_DEVICE_H

#include <cassert>
#include <common/align.hpp>
#include <common/units.hpp>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>

namespace mdfs {
class BlockDevice {
public:
	BlockDevice() {}
	BlockDevice(std::string path, size_t blockSize = 512,
				std::ios_base::openmode openmode = std::ios::in | std::ios::out) {
		open(path, blockSize, openmode);
	}
	~BlockDevice() {
		if (m_file.is_open()) { m_file.close(); }
	}

	void open(std::string path, size_t blockSize, std::ios_base::openmode openmode) {
		m_openmode = openmode;
		m_blockSize = blockSize;
		m_file.open(path, std::ios::in | std::ios::out | std::ios::binary);
		if (!m_file.is_open()) { throw std::runtime_error("Failed to open image"); }
		m_file.seekg(0, std::ios::end);
		m_fileSize = mdfs::align_down<size_t>(m_file.tellg(), m_blockSize);
		m_file.seekg(0);
		if (!m_file.is_open()) { throw std::runtime_error("Failed to open image"); }
	}

	void close() {
		if (m_file.is_open()) { m_file.close(); }
		m_blockSize = 512;
		m_fileSize = 0;
	}

	void flush() { m_file.flush(); }

	void seekg(size_t LBA) {
		if (!(m_openmode & std::ios::in)) { return; }
		assert(LBA < size_lba());
		m_file.seekg(mdfs::lba_to_addr(LBA, m_blockSize));
	}
	void seekp(size_t LBA) {
		if (!(m_openmode & std::ios::out)) { return; }
		assert(LBA < size_lba());
		m_file.seekp(mdfs::lba_to_addr(LBA, m_blockSize));
	}

	size_t tellg() {
		if (!(m_openmode & std::ios::in)) { return 0; }
		return mdfs::addr_to_lba(m_file.tellg(), m_blockSize);
	}
	size_t tellp() {
		if (!(m_openmode & std::ios::out)) { return 0; }
		return mdfs::addr_to_lba(m_file.tellp(), m_blockSize);
	}

	void read(char *data, size_t size) {
		if (size == 0) { return; }
		if (!(m_openmode & std::ios::in)) { return; }
		size_t sizeInLBA = mdfs::align_up(size, m_blockSize) / m_blockSize;
		char *tempData = new char[sizeInLBA * m_blockSize];
		read_lba(tempData, sizeInLBA);
		memcpy(data, tempData, size);
		delete[] tempData;
	}
	void write(const char *data, size_t size) {
		if (size == 0) { return; }
		if (!(m_openmode & std::ios::out)) { return; }
		size_t sizeInLBA = mdfs::align_up(size, m_blockSize) / m_blockSize;
		char *tempData = new char[sizeInLBA * m_blockSize];
		memcpy(tempData, data, size);
		memset(tempData + size, 0x00, (sizeInLBA * m_blockSize) - size);
		write_lba(tempData, sizeInLBA);
		delete[] tempData;
	}


	void read_lba(char *data, size_t sizeInLBA) {
		if (sizeInLBA == 0) { return; }
		if (!(m_openmode & std::ios::in)) { return; }
		auto pos = m_file.tellg();
		assert(pos >= 0);
		assert((sizeInLBA + pos / m_blockSize) <= size_lba());
		m_file.read(data, mdfs::lba_to_addr(sizeInLBA, m_blockSize));
	}
	void write_lba(const char *data, size_t sizeInLBA) {
		if (sizeInLBA == 0) { return; }
		if (!(m_openmode & std::ios::out)) { return; }
		auto pos = m_file.tellp();
		assert(pos >= 0);
		assert((sizeInLBA + pos / m_blockSize) <= size_lba());
		m_file.write(data, mdfs::lba_to_addr(sizeInLBA, m_blockSize));
	}

	void read_lba(size_t LBA, char *data, size_t sizeInLBA) {
		if (sizeInLBA == 0) { return; }
		if (!(m_openmode & std::ios::in)) { return; }
		assert((LBA + sizeInLBA) <= size_lba());
		size_t currentPos = m_file.tellg();
		m_file.seekg(mdfs::lba_to_addr(LBA, m_blockSize));
		m_file.read(data, mdfs::lba_to_addr(sizeInLBA, m_blockSize));
		m_file.seekg(currentPos);
	}
	void write_lba(size_t LBA, const char *data, size_t sizeInLBA) {
		if (sizeInLBA == 0) { return; }
		if (!(m_openmode & std::ios::out)) { return; }
		assert((LBA + sizeInLBA) <= size_lba());
		size_t currentPos = m_file.tellp();
		m_file.seekp(mdfs::lba_to_addr(LBA, m_blockSize));
		m_file.write(data, sizeInLBA * m_blockSize);
		m_file.seekp(currentPos);
	}

	size_t size_lba() { return m_fileSize / m_blockSize; }
	size_t size_b() { return m_fileSize; }
	size_t block_size() { return m_blockSize; }
	void set_block_size(size_t newSize) { m_blockSize = newSize; }

private:
	std::fstream m_file;
	size_t m_blockSize = 512;
	size_t m_fileSize = 0;
	std::ios_base::openmode m_openmode;
};
}// namespace mdfs

#endif