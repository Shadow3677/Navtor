#include "FileManager.hpp"
#include "Logger.hpp"

#include <openssl/evp.h>
#include <zlib.h>
#include <chrono>
#include <unordered_map>

namespace
{
    constexpr char MAGIC[4] = { 'T','M','A','R' };
    constexpr uint32_t VERSION = 2;
} // anonymous namespace


/**
* Name: Compressor::Compressor
* Description: Constructor
* @Param compresor - compresor
*/
FileManager::FileManager(Compressor& compresor) : m_compressor(compresor) {}
/**
* Name: FileManager::scanFiles
* Description: gather files recursively, compute SHA-256 and metadata
* @Param root - absolute path to root directory
*/
std::vector<FileMetadata> FileManager::scanFiles(const fs::path& root)
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
            entries.push_back(fm);
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
* Name: FileManager::Pack
* Description: Pack files from the root directory into an archive
* @Param root - absolute path to root directory
* @Param archivePath - absolute path to the output archive location
*/
void FileManager::Pack(const fs::path& root, const fs::path& archivePath)
{
    LOG(Info, "Entry.");

    auto files = scanFiles(root);

    if (files.empty())
    {
        LOG(Info, "No file to compress.");
        return;
    }

    std::ofstream ofStream(archivePath, std::ios::binary);
    if (!ofStream)
    {
        LOG(Error, "Cannot create archive.");
        return;
    }

    std::unordered_map<std::string, fs::path> uniqueFiles;
    for (FileMetadata& file : files)
    {
        uniqueFiles[file.sha256] = fs::absolute(root / file.path);
    }

    ofStream.write(MAGIC, 4);
    write_u32(ofStream, VERSION);
    write_u32(ofStream, static_cast<uint32_t>(uniqueFiles.size()));
    write_u32(ofStream, static_cast<uint32_t>(files.size()));

    //bloobs: SHA, orginal size, compressed size, data
    for (auto& [sha, path] : uniqueFiles)
    {
        char shaBin[32];
        hexToBinSHA(shaBin, sha);
        ofStream.write(shaBin, 32);

        uint64_t originalSize = fs::file_size(path);
        write_u64(ofStream, originalSize);

        std::stringstream compressedData;
        uint64_t compressedSize = m_compressor.compressFileToStream(path, compressedData);
        write_u64(ofStream, compressedSize);

        ofStream << compressedData.rdbuf();
    }

    for (auto& file : files)
    {
        uint32_t pathLength = static_cast<uint32_t>(file.path.size());
        write_u32(ofStream, pathLength);
        ofStream.write(file.path.data(), pathLength);

        char shaBin[32];
        hexToBinSHA(shaBin, file.sha256);
        ofStream.write(shaBin, 32);

        write_u64(ofStream, file.size);
        write_u32(ofStream, file.readonly ? 1 : 0);
        write_u64(ofStream, file.time);
    }

    ofStream.close();
    LOG(Info, "Exit.");
}

/**
* Name: FileManager::sha256File
* Description: Calculate SHA256 for the given file
* @Param path - absolute path to file
*/
std::string FileManager::sha256File(const fs::path& path)
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

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;

    if (1 != EVP_DigestFinal_ex(mdCtx, hash, &hash_len))
    {
        LOG(Error, "Update failed.");
        return std::string();
    }

    EVP_MD_CTX_free(mdCtx);
    
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');

    for (unsigned int i = 0; i < hash_len; ++i)
    {
        ss << std::setw(2) << static_cast<int>(hash[i]);
    }

    return ss.str();
}

/**
* Name: FileManager::Unpack
* Description: Unpackl files from the archive into an directory
* @Param archivePath - absolute path to the archive
* @Param destRoot - absolute path to the root directory
*/
void FileManager::Unpack(const fs::path& archivePath, const fs::path& destRoot)
{
    LOG(Info, "Entry.");

    std::ifstream ifstream(archivePath, std::ios::binary);
    if (!ifstream)
    {
        LOG(Error, "Cannot open archive.");
        return;
    }

    char magic[4];
    ifstream.read(magic, 4);
    if (memcmp(magic, MAGIC, 4) != 0)
    {
        LOG(Error, "Invalid archive magic.");
        return;
    }

    uint32_t version = read_u32(ifstream);
    uint32_t numBlobs = read_u32(ifstream);
    uint32_t numFiles = read_u32(ifstream);

    struct Blob
    {
        uint64_t origSize;
        uint64_t compSize;
        std::streampos pos;
    };

    std::unordered_map<std::string, Blob> blobMap(numBlobs);

    //read blobs
    for (uint32_t i = 0; i < numBlobs; i++)
    {
        std::ostringstream shaHex;
        binToHexSHA(ifstream, shaHex);

        uint64_t origSize = read_u64(ifstream);
        uint64_t compSize = read_u64(ifstream);
        std::streampos pos = ifstream.tellg();

        ifstream.seekg(static_cast<std::streamoff>(compSize), std::ios::cur);
        if (!ifstream)
        {
            LOG(Error, "Corrupted archive while skipping blob data.");
            return;
        }

        blobMap.emplace(shaHex.str(), Blob{ origSize, compSize, pos });
    }

    // Read metaddata and create files
    for (uint32_t i = 0; i < numFiles; i++)
    {
        uint32_t pathLen = read_u32(ifstream);
        std::string relpath(pathLen, '\0');
        if (!ifstream.read(&relpath[0], pathLen))
        {
            LOG(Error, "Corrupted archive while reading path.");
            return;
        }

        std::ostringstream shaHex;
        binToHexSHA(ifstream, shaHex);

        uint64_t size = read_u64(ifstream);
        uint32_t readonly = read_u32(ifstream);
        uint64_t mtime = read_u64(ifstream);

        auto it = blobMap.find(shaHex.str());
        if (it == blobMap.end())
        {
            LOG(Error, "Missing blob for file: %s", relpath.c_str());
            return;
        }

        const Blob& blob = it->second;

        // save metadata pos
        std::streampos metadata_pos = ifstream.tellg();

        // go to blobs
        ifstream.clear();
        ifstream.seekg(blob.pos);
        if (!ifstream)
        {
            LOG(Error, "Seekg failed for blob: %s", relpath.c_str());
            return;
        }

        fs::path outPath = destRoot / relpath;
        fs::create_directories(outPath.parent_path());

        m_compressor.decompresStreamToFile(ifstream, blob.compSize, outPath);
        // back to metadata
        ifstream.clear();
        ifstream.seekg(metadata_pos);

        if (readonly)
        {
            auto perms = fs::status(outPath).permissions();
            fs::permissions(outPath, perms & ~fs::perms::owner_write);
        }

        auto ftime = fs::file_time_type::clock::time_point(std::chrono::seconds(mtime));
        fs::last_write_time(outPath, ftime);
    }

    LOG(Info, "Exit.");
}

/**
* Name: FileManager::binToHexSHA
* Description: Reads and convers bin to hex SHA
* @Param ifstream - file stream with binary SHA
* @param shaHex - string stream with hex SHA
*/
void FileManager::binToHexSHA(std::ifstream& ifstream, std::ostringstream& shaHex)
{
    unsigned char shaBin[32];
    if (!ifstream.read(reinterpret_cast<char*>(shaBin), 32))
    {
        LOG(Error, "Corrupted archive while reading file SHA.");
        return;
    }

    shaHex << std::hex << std::setfill('0');
    for (int j = 0; j < 32; j++)
    {
        shaHex << std::setw(2) << static_cast<int>(shaBin[j]);
    }
}

/**
* Name: FileManager::hexToBinSHA
* Description: Convers hex to bin SHA
* @Param shaBin - output shaBin
* @Param shaHex - string containing hex SHA
*/
void FileManager::hexToBinSHA(char* shaBin, const std::string& shaHex)
{
    for (int i = 0; i < 32; i += 2)
    {
        shaBin[i / 2] = static_cast<char>(std::stoi(shaHex.substr(i, 2), nullptr, 16));
    }
}
