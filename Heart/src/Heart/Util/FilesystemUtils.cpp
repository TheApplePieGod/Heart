#include "htpch.h"
#include "FilesystemUtils.h"

namespace Heart
{
    std::string FilesystemUtils::LoadFile(const std::string& path)
    {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            HE_ENGINE_LOG_ERROR("Failed to load file {0}", path);
            HE_ENGINE_ASSERT(false);
        }
        
        u64 fileSize = file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return std::string(buffer.data(), buffer.size());
    }
}