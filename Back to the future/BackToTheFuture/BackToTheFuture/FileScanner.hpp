#pragma once

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

struct FileMetadata
{
	std::string path;
	std::string sha256;
	std::size_t size;
	bool readonly;
	int64_t time;
};

class FileScanner
{
public:
	std::vector<FileMetadata> scanFiles(const fs::path& root);

private:
	std::string sha256File(const fs::path& path);

};