#include <iostream>
#include <thread>
#include "FileManager.hpp"

namespace fs = std::filesystem;

namespace
{
    const char* PACK_MODE = "pack";
    const char* UNPACK_MODE = "unpack";
} //anonymous namespace

void printHelp()
{
    std::cout << "Usage: app pack <input_folder> <archive_path>\n"
        << "       app unpack <archive_path> <output_folder>\n";
}

int main(int argc, char** argv)
{
    if (4 > argc)
    {
        printHelp();
        return 0;
    }

    Compressor compressor;
    FileManager fileManager(compressor);
    std::string mode(argv[1]);
    if (PACK_MODE == mode)
    {
        fs::path inputFolder = argv[2];
        fs::path archiveFile = argv[3];

        std::thread worker([&]()
            {
                std::cout << "Start packing\n";
                fileManager.Pack(inputFolder, archiveFile);
            });

        worker.join();
        std::cout << "Packing Finished\n";
    }
    else if (UNPACK_MODE == mode)
    {
        fs::path archiveFile = argv[2];
        fs::path outputFolder = argv[3];

        std::thread worker([&]()
            {
                std::cout << "Start unpacking\n";
                fileManager.Unpack(archiveFile, outputFolder);
            });

        worker.join();
        std::cout << "Unpacking Finished\n";
    }
    else
    {
        std::cout << "Unknow Method";
    }
}
