#include "hepch.h"
#include "SplatAsset.h"

#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Util/StringUtils.hpp"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Asset/TextureAsset.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace Heart
{
    void SplatAsset::LoadInternal()
    {
        HE_PROFILE_FUNCTION();

        u32 fileLength;
        unsigned char* data = nullptr;

        try
        {
            data = FilesystemUtils::ReadFile(m_AbsolutePath, fileLength);
            if (!data)
                throw std::exception();


        }
        catch (std::exception e)
        {
            HE_ENGINE_LOG_ERROR("Failed to load splat at path {0}", m_AbsolutePath.Data());
            return;
        }

        delete[] data;
        m_Valid = true;
    }

    void SplatAsset::UnloadInternal()
    {
        m_Data = nullptr;
    }
}
