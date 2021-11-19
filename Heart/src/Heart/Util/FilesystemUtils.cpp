#include "htpch.h"
#include "FilesystemUtils.h"

namespace Heart
{
    std::string FilesystemUtils::ReadFileToString(const std::string& path)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open())
            return "";
        
        u32 fileSize = static_cast<u32>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), fileSize);
        file.close();

        return std::string(buffer.data(), buffer.size());
    }

    unsigned char* FilesystemUtils::ReadFile(const std::string& path, u32& outLength)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open())
            return nullptr;
        
        u32 fileSize = static_cast<u32>(file.tellg());
        unsigned char* buffer = new unsigned char[fileSize + 1];
        file.seekg(0, std::ios::beg);
        file.read((char*)buffer, fileSize);
        buffer[fileSize] = 0;
        file.close();

        outLength = fileSize;
        return buffer;
    }
}