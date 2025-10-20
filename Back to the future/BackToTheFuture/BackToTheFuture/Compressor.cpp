#include "Compressor.hpp"
#include "Logger.hpp"

/**
* Name: Compressor::Compressor
* Description: Constructor
* @Param chunkSize - chunk size
*/
Compressor::Compressor(std::size_t chunkSize) :
	m_CHUNK(chunkSize), inBuffer(chunkSize), outBuffer(chunkSize) {}

/**
* Name: Compressor::Compressor::ScopedZStream::ScopedZStream
* Description: Constructor
* @Param stream - zStream from zlib
* @Param level - compresion level
* @param mode - compression or decompression mode
*/
Compressor::ScopedZStream::ScopedZStream(z_stream& stream, int level, Mode mode) :
	m_zs(stream), m_mode(mode) {}

/**
* Name: Compressor::Compressor::ScopedZStream::ScopedZStream
* Description: Constructor
* @Param stream - zStream from zlib
* @param mode - compression or decompression mode
*/
Compressor::ScopedZStream::ScopedZStream(z_stream& stream, Mode mode) :
	m_zs(stream), m_mode(mode) {}

/**
* Name: Compressor::ScopedZStream::~ScopedZStream
* Description: Deconstructor
* @Param chunkSize - chunk size
*/
Compressor::ScopedZStream::~ScopedZStream()
{
	LOG(Info, "Entry, mode: %d.", m_mode);
	if (Mode::Deflate == m_mode)
	{
		deflateEnd(&m_zs);
	}
	else
	{
		inflateEnd(&m_zs);
	}
	LOG(Info, "Exit.");
}

/**
* Name: Compressor::compressFileToStream
* Description: Compress file, chunk by chunk, to stream using zlib
* @Param path - absolute path to file
* @Param ostream - output stream
*/
uint64_t Compressor::compressFileToStream(const fs::path& path, std::ostream& ostream)
{
	LOG(Info, "Entry.");

	std::ifstream inFile(path, std::ios::binary);
	if (!inFile)
	{
		LOG(Error, "Cannot open file %s.", path.string().c_str());
		return 0;
	}

	z_stream zStream{};
	Compressor::ScopedZStream deflateGuard(zStream, Z_BEST_COMPRESSION, Compressor::ScopedZStream::Mode::Deflate);
	if (Z_OK != deflateInit(&zStream, Z_BEST_COMPRESSION))
	{
		LOG(Error, "deflateInit failed.");
		return 0;
	}
	
	uint64_t totalOut = 0;
	int flush = Z_NO_FLUSH;

	while (inFile)
	{
		inFile.read(reinterpret_cast<char*>(inBuffer.data()), m_CHUNK);
		std::streamsize readBytes = inFile.gcount();
		if (0 >= readBytes)
		{
			break;
		}

		flush = inFile.eof() ? Z_FINISH : Z_NO_FLUSH;

		zStream.next_in = reinterpret_cast<Bytef*>(inBuffer.data());
		zStream.avail_in = static_cast<uInt>(readBytes);

		do
		{
			zStream.next_out = reinterpret_cast<Bytef*>(outBuffer.data());
			zStream.avail_out = static_cast<uInt>(m_CHUNK);

			if (Z_STREAM_ERROR == deflate(&zStream, flush))
			{
				LOG(Error, "Deflate error.");
				return 0;
			}

			uInt have = static_cast<uInt>(m_CHUNK) - zStream.avail_out;
			ostream.write(reinterpret_cast<char*>(outBuffer.data()), have);
			totalOut += have;
		} while (0 == zStream.avail_out);
	}

	LOG(Info, "Exit.");
	return totalOut;
}

/**
* Name: Compressor::decompresStreamToFile
* Description: decompres file stream to file
* @Param path - absolute path to file
* @Param ostream - output stream
*/
void Compressor::decompresStreamToFile(std::istream& istream, uint64_t compressedSize, const fs::path& outPath)
{
	LOG(Info, "Entry.");

	z_stream zStream{};
	ScopedZStream inflateGuard(zStream, Compressor::ScopedZStream::Mode::Inflate);
	if (Z_OK != inflateInit(&zStream))
	{
		LOG(Error, "InflateInit failed.");
		return;
	}

	std::ofstream outFile(outPath, std::ios::binary);
	if (!outFile)
	{
		LOG(Error, "Cannot create output file %s.", outPath.string().c_str());
		return;
	}

	uint64_t remaining = compressedSize;

	while (0 < remaining)
	{
		int toRead = static_cast<int>(std::min<uint64_t>(m_CHUNK, remaining));

		istream.read(reinterpret_cast<char*>(inBuffer.data()), toRead);
		int got = static_cast<int>(istream.gcount());
		if (0 >= got)
		{
			LOG(Error, "Unexpected EOF.");
			return;
		}

		remaining -= got;

		zStream.next_in = reinterpret_cast<Bytef*>(inBuffer.data());
		zStream.avail_in = got;

		do
		{
			zStream.next_out = reinterpret_cast<Bytef*>(outBuffer.data());
			zStream.avail_out = static_cast<uInt>(m_CHUNK);

			int ret = inflate(&zStream, Z_NO_FLUSH);
			if (0 > ret)
			{
				LOG(Error, "Inflate error.");
				return;
			}

			uInt have = static_cast<uInt>(m_CHUNK) - zStream.avail_out;
			if (0 < have)
			{
				outFile.write(reinterpret_cast<char*>(outBuffer.data()), have);
			}

		} while (0 == zStream.avail_out);
	}

	LOG(Info, "Exit.");
}
