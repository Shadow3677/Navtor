#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "Logger.hpp"
#include "Compressor.hpp"

namespace fs = std::filesystem;

struct FileMetadata
{
	std::string path;
	std::string sha256;
	std::size_t size;
	bool readonly;
	int64_t time;
};

class IFileManager
{
public: 
	virtual void Pack(const fs::path& root, const fs::path& archivePath) = 0;
	virtual void Unpack(const fs::path& archivepath, const fs::path& destRoot) = 0;
};

class FileManager : IFileManager
{
public:
	explicit FileManager(Compressor& compresor);
	void Pack(const fs::path& root, const fs::path& archivePath) override;
	void Unpack(const fs::path& archivepath, const fs::path& destRoot) override;

private:
	inline void write_u32(std::ostream& os, uint32_t v) { for (int i = 0; i < 4; i++) os.put((char)((v >> (8 * i)) & 0xFF)); }
	inline void write_u64(std::ostream& os, uint64_t v) { for (int i = 0; i < 8; i++) os.put((char)((v >> (8 * i)) & 0xFF)); }
	inline uint32_t read_u32(std::istream& is) { uint32_t v = 0; for (int i = 0; i < 4; i++) { int c = is.get(); if (c == EOF) LOG(Error, "Unexpected EOF"); v |= (uint32_t)c << (8 * i); } return v; }
	inline uint64_t read_u64(std::istream& is) { uint64_t v = 0; for (int i = 0; i < 8; i++) { int c = is.get(); if (c == EOF) LOG(Error, "Unexpected EOF"); v |= (uint64_t)c << (8 * i); } return v; }
	
	std::vector<FileMetadata> scanFiles(const fs::path& root);
	std::string sha256File(const fs::path& path);

	void binToHexSHA(std::ifstream& ifstream, std::ostringstream& shaHex);
	void hexToBinSHA(char* shaBin, const std::string& shaHex);

private:
	Compressor& m_compressor;
};
