#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <zlib.h>

namespace fs = std::filesystem;

class Compressor
{
public:
	explicit Compressor(std::size_t chunkSize = 1 << 20);
	uint64_t compressFileToStream(const fs::path& path, std::ostream& ostream);
	void decompresStreamToFile(std::istream& istream, uint64_t compressedSize, const fs::path& outPath);

private:
	const std::size_t m_CHUNK;
	std::vector<char> inBuffer;
	std::vector<char> outBuffer;

	struct ScopedZStream
	{
		enum class Mode { Deflate = 0, Inflate };

		ScopedZStream(z_stream& stream, int level, Mode mode);
		ScopedZStream(z_stream& stream, Mode mode);
		~ScopedZStream();

		z_stream m_zs;
		Mode m_mode;
	};
};