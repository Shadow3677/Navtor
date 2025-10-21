#include "FileScanner.hpp"
#include "Logger.hpp"

#include <openssl/evp.h>

/**
* Name: FileScanner::scanFiles
* Description: gather files recursively, compute SHA-256 and metadata
* @Param root - absolute path to root directory
*/
std::vector<FileMetadata> FileScanner::scanFiles(const fs::path& root)
{
	LOG(Info, "Entry.");

	std::vector<FileMetadata> entries;

	if (root.empty())
	{
		LOG(Debug, "root is empty");
		return entries;
	}

	try
	{
		for (const auto& entry : fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied))
		{
			if (!fs::is_regular_file(entry))
			{
				continue;
			}

			FileMetadata fm;
			fm.path = fs::relative(entry.path(), root).generic_string();
			fm.sha256 = sha256File(entry.path());
			fm.size = fs::file_size(entry);
			fm.readonly = (fs::status(entry).permissions() & fs::perms::owner_write) == fs::perms::none;
			auto fTime = fs::last_write_time(entry);
			fm.time = std::chrono::duration_cast<std::chrono::seconds>(fTime.time_since_epoch()).count();
			entries.emplace_back(fm);
		}

		std::sort(entries.begin(), entries.end(), [](const FileMetadata& rhs, const FileMetadata& lhs) { return rhs.path < lhs.path; });
		return entries;
	}
	catch (const fs::filesystem_error& error)
	{
		std::cerr << "Error: " << error.what() << "\n";
	}

	LOG(Info, "Exit.");
	return entries;
}

/**
* Name: FileScanner::sha256File
* Description: Calculate SHA256 for the given file
* @Param path - absolute path to file
*/
std::string FileScanner::sha256File(const fs::path& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		LOG(Error, "Failed to open file: %s.", path.string().c_str());
		return std::string();
	}

	EVP_MD_CTX* mdCtx = EVP_MD_CTX_new();
	if (!mdCtx)
	{
		LOG(Error, "Failed to create EVP context");
		return std::string();
	}

	if (1 != EVP_DigestInit_ex(mdCtx, EVP_sha256(), nullptr))
	{
		LOG(Error, "Failed to init EVP.");
		return std::string();
	}

	const std::size_t bufferSize = 1 << 20;

	std::vector<char> buffer(bufferSize);

	while (file)
	{
		file.read(buffer.data(), bufferSize);
		std::streamsize streamSize = file.gcount();

		if (0 < streamSize)
		{
			if (1 != EVP_DigestUpdate(mdCtx, buffer.data(), streamSize))
			{
				LOG(Error, "Update failed.");
			}
		}
	}

	std::vector<unsigned char> hash(EVP_MAX_MD_SIZE);
	unsigned int hashLen = 0;

	if (1 != EVP_DigestFinal_ex(mdCtx, hash.data(), &hashLen))
	{
		LOG(Error, "Update failed.");
		return std::string();
	}

	EVP_MD_CTX_free(mdCtx);

	std::ostringstream ss;
	ss << std::hex << std::setfill('0');

	for (unsigned int i = 0; i < hashLen; ++i)
	{
		ss << std::setw(2) << static_cast<int>(hash[i]);
	}

	return ss.str();
}
