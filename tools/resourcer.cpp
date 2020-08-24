
#include <fstream>
#include <ghc/fs_std.hpp>
#include <iostream>
#include <map>
#include <string>

typedef std::map<std::string, long> FileInfos;

std::string lineBuffer;
int globalCount = 0;

void writeByte(std::ostream& stream, unsigned char byte)
{
    lineBuffer += std::to_string((int)byte) + ",";
    ++globalCount;
    if (lineBuffer.length() > 75) {
        stream << lineBuffer << std::endl;
        lineBuffer.clear();
    }
}
void writeInt(std::ostream& stream, int val)
{
    writeByte(stream, (unsigned char)(val & 0xff));
    writeByte(stream, (unsigned char)((val >> 8) & 0xff));
    writeByte(stream, (unsigned char)((val >> 16) & 0xff));
    writeByte(stream, (unsigned char)((val >> 24) & 0xff));
}
void writeString(std::ostream& stream, const std::string& str)
{
    for (unsigned i = 0; i < str.size(); ++i)
        writeByte(stream, (unsigned char)(str[i]));
}

int main(int argc, char* argv[])
{
    FileInfos files;
    int totalSize = 0;
    int filenamesSize = 0;
    if (argc < 3) {
        puts("USAGE: resourcer <directory> <outputfile>");
        exit(-1);
    }

    std::string dir = argv[1];
    if (!dir.empty() && dir[dir.length() - 1] != '/' && dir[dir.length() - 1] != '\\') {
        dir += "/";
    }

    fs::path inputDir = fs::canonical(dir);
    std::clog << "Reading content of '" << inputDir.string() << "' ..." << std::endl;

    for (fs::recursive_directory_iterator dir(inputDir); dir != fs::recursive_directory_iterator(); ++dir) {
        std::string filepath = dir->path().string().substr(inputDir.string().length() + 1);
        if (!filepath.empty() && filepath[0] != '.' && fs::is_regular_file(*dir)) {
            long size = (long)fs::file_size(*dir);
            std::cout << filepath << " (" << size << ")" << std::endl;
            files[filepath] = size;
            totalSize += size;
            filenamesSize += filepath.length();
        }
    }

    std::clog << "Found " << files.size() << " files with " << totalSize << " bytes of data, processing..." << std::endl;

    std::ofstream output(argv[2], std::ios::trunc);
    int resourceDataSize = (totalSize + 4 + files.size() * 12 + filenamesSize + 1);
    output << "const int g_resourceDataSize = " << resourceDataSize << ";" << std::endl;
    output << "const unsigned char g_resourceData[" << resourceDataSize << "] = {" << std::endl;
    writeInt(output, files.size());
    int offset = 4 + files.size() * 4;
    for (FileInfos::const_iterator iter = files.begin(); iter != files.end(); ++iter) {
        writeInt(output, offset);
        offset += iter->second + 8 + iter->first.length();
    }
    int checkByteNum = 0;
    offset = 4 + files.size() * 4;
    for (FileInfos::const_iterator iter = files.begin(); iter != files.end(); ++iter) {
        std::clog << "packing '" << iter->first << "' ..." << std::endl;
        if (globalCount != offset) {
            std::clog << "    error: expected offset " << offset << " current offset " << globalCount << " !!!" << std::endl;
        }
        writeInt(output, iter->second);
        writeInt(output, iter->first.length());
        writeString(output, iter->first);
        std::ifstream file((std::string(dir) + iter->first).c_str(), std::ios::binary);
        int c;
        while (!file.eof()) {
            c = file.get();
            if (!file.eof()) {
                writeByte(output, (unsigned char)c);
                ++checkByteNum;
            }
        }
        std::clog << " imported " << iter->first << " with " << checkByteNum << " bytes" << std::endl;
        offset += iter->second + 8 + iter->first.length();
        checkByteNum = 0;
    }
    output << lineBuffer << "0" << std::endl;
    output << "};" << std::endl;
    output.close();
    std::clog << globalCount << " / " << offset << " bytes done." << std::endl;
    return 0;
}