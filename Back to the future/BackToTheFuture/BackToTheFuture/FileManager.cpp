#include "FileManager.hpp"
#include "Logger.hpp"

#include <openssl/evp.h>
#include <zlib.h>
#include <chrono>
#include <unordered_map>

namespace {
    constexpr int CHUNK = 1 << 20; // 1MB chunk
    constexpr char MAGIC[4] = { 'T','M','A','R' };
    constexpr uint32_t VERSION = 2;
}

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
        goto exit;
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

exit:
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
    std::unordered_map<std::string, fs::path> uniqueFiles;
    
    for (FileMetadata& file : files)
    {
        uniqueFiles[file.sha256] = fs::absolute(root / file.path);
    }

    std::ofstream ofStream(archivePath, std::ios::binary);
    if (!ofStream)
    {
        LOG(Error, "Cannot create archive.");
        goto Exit;
    }

    ofStream.write(MAGIC, 4);
    write_u32(ofStream, VERSION);
    write_u32(ofStream, static_cast<uint32_t>(uniqueFiles.size()));
    write_u32(ofStream, static_cast<uint32_t>(files.size()));

    //bloobs: SHA, orginal size, compressed size, data

    for (auto& [sha, path] : uniqueFiles)
    {
        char shaBin[32];
        for (int i = 0; i < 32; i += 2)
        {
            shaBin[i/2] = static_cast<char>(std::stoi(sha.substr(i,2), nullptr, 16));
        }

        ofStream.write(shaBin, 32);

        uint64_t originalSize = fs::file_size(path);
        write_u64(ofStream, originalSize);

        std::stringstream compressedData;
        uint64_t compressedSize = compressFileToStream(path, compressedData);
        write_u64(ofStream, compressedSize);

        ofStream << compressedData.rdbuf();
    }

    for (auto& file : files)
    {
        uint32_t pathLength = static_cast<uint32_t>(file.path.size());
        write_u32(ofStream, pathLength);
        ofStream.write(file.path.data(), pathLength);

        char shaBin[32];
        for (int i = 0; i < 32; i += 2)
        {
            shaBin[i / 2] = static_cast<char>(std::stoi(file.sha256.substr(i,2), nullptr, 16));
        }
        ofStream.write(shaBin, 32);

        write_u64(ofStream, file.size);
        write_u32(ofStream, file.readonly ? 1 : 0);
        write_u64(ofStream, file.time);
    }

Exit:
    ofStream.close();
    LOG(Info, "Exit.");
}

/**
* Name: FileManager::compressFileToStream
* Description: Compress file, chunk by chunk, to stream using zlib
* @Param path - absolute path to file
* @Param ostream - output stream
*/
uint64_t FileManager::compressFileToStream(const fs::path& path, std::ostream& ostream)
{
    LOG(Info, "Entry.");

    std::ifstream inFile(path, std::ios::binary);
    int flush;
    std::vector<char> in(CHUNK), out(CHUNK);
    z_stream zStream{};
    uint64_t totalOut = 0;

    if (!inFile)
    {
        LOG(Error, "Cannot open file %s.", path.string().c_str());
        goto Error;
    }
    
    if (Z_OK != deflateInit(&zStream, Z_BEST_COMPRESSION))
    {
        LOG(Error, "deflateInit failed.");
        goto Error;
    }

    do {
        inFile.read(in.data(), CHUNK);
        std::streamsize readBytes = inFile.gcount();
        flush = inFile.eof() ? Z_FINISH : Z_NO_FLUSH;
        zStream.next_in = reinterpret_cast<Bytef*>(in.data());
        zStream.avail_in = static_cast<uInt>(readBytes);

       do {
            zStream.next_out = reinterpret_cast<Bytef*>(out.data());
            zStream.avail_out = CHUNK;

            if (Z_STREAM_ERROR == deflate(&zStream, flush))
            {
                LOG(Error, "Deflat error.");
                goto Error;
            }

            uInt have = CHUNK - zStream.avail_out;
            ostream.write(out.data(), have);
            totalOut += have;
       } while (zStream.avail_out == 0);
    } while (flush != Z_FINISH);

    deflateEnd(&zStream);

    LOG(Info, "Exit.");
    return totalOut;

Error:
    deflateEnd(&zStream);
    return 0;
}

/**
* Name: FileManager::sha256File
* Description: Calculate SHA256 for the given file
* @Param path - absolute path to file
*/
std::string FileManager::sha256File(const fs::path& path)
{
    LOG(Info, "Entry.");
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

    LOG(Info, "Exit.");
    return ss.str();
}
