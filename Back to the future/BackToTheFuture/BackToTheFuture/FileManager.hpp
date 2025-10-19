#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

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
};

class FileManager : IFileManager
{
public:
	
	void Pack(const fs::path& root, const fs::path& archivePath) override;
private:

	inline void write_u32(std::ostream& os, uint32_t v) { for (int i = 0; i < 4; i++) os.put((char)((v >> (8 * i)) & 0xFF)); }
	inline void write_u64(std::ostream& os, uint64_t v) { for (int i = 0; i < 8; i++) os.put((char)((v >> (8 * i)) & 0xFF)); }
	
	std::vector<FileMetadata> scanFiles(const fs::path& root);
	uint64_t compressFileToStream(const fs::path& path, std::ostream& ostream);
	std::string sha256File(const fs::path& path);
};