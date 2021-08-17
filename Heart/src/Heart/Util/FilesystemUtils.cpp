#include "htpch.h"
#include "FilesystemUtils.h"

namespace Heart
{
    std::string FilesystemUtils::LoadFile(const std::string& path)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        HT_ENGINE_ASSERT(file.is_open(), "Failed to load file ${1}", path);
        
        u64 fileSize = file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return std::string(buffer.data(), buffer.size());
    }
}