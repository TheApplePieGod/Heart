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

            ParseSplat(data, fileLength);
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

    // https://github.com/antimatter15/splat/blob/main/main.js
    void SplatAsset::ParseSplat(unsigned char* data, u32 length)
    {
        // XYZ - Position (Float32)
        // XYZ - Scale (Float32)
        // RGBA - colors (uint8)
        // IJKL - quaternion/rot (uint8)
        // 32 bytes total
        u32 vertexLength = 3 * 4 + 3 * 4 + 4 + 4;
        u32 vertexCount = length / vertexLength;

        HVector<glm::mat4> transforms;
        HVector<glm::vec4> colors;
        float* floatData = (float*)data;
        for (u32 i = 0; i < vertexCount; i++)
        {
            //1499000

            glm::vec3 scale = {
                floatData[8 * i + 3 + 0],
                floatData[8 * i + 3 + 1],
                floatData[8 * i + 3 + 2],
            };

            glm::vec3 vertPos = {
                floatData[8 * i + 0],
                floatData[8 * i + 1],
                floatData[8 * i + 2]
            };

            glm::quat quat = {
                (data[32 * i + 28 + 0] - 128) / 128.f,
                (data[32 * i + 28 + 1] - 128) / 128.f,
                (data[32 * i + 28 + 2] - 128) / 128.f,
                (data[32 * i + 28 + 3] - 128) / 128.f,
            };

			transforms.AddInPlace(
                glm::translate(glm::mat4(1.0f), vertPos)
                    * glm::scale(glm::mat4(1.0f), scale)
                    * glm::toMat4(quat)
            );

            colors.AddInPlace(
                data[32 * i + 24 + 0] / 255.f,
                data[32 * i + 24 + 1] / 255.f,
                data[32 * i + 24 + 2] / 255.f,
                data[32 * i + 24 + 3] / 255.f
            );

            /*
            HE_LOG_WARN(
                "X: {0}, Y: {1}, Z: {2}",
                scale.x,
                scale.y,
                scale.z
            );

            HE_LOG_WARN(
                "R: {0}, G: {1}, B: {2}, A: {3}",
                colors.Back().r,
                colors.Back().g,
                colors.Back().b,
                colors.Back().a
            );
            */

            //if (i > 1500000)
            //    break;
        }

        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Type = Flourish::BufferType::Storage;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Static;
        bufCreateInfo.Stride = sizeof(glm::mat4);
        bufCreateInfo.ElementCount = transforms.Count();
        bufCreateInfo.InitialData = transforms.Data();
        bufCreateInfo.InitialDataSize = sizeof(glm::mat4) * transforms.Count();
        m_TransformBuffer = Flourish::Buffer::Create(bufCreateInfo);

        bufCreateInfo.Stride = sizeof(glm::vec4);
        bufCreateInfo.ElementCount = colors.Count();
        bufCreateInfo.InitialData = colors.Data();
        bufCreateInfo.InitialDataSize = sizeof(glm::vec4) * colors.Count();
        m_ColorBuffer = Flourish::Buffer::Create(bufCreateInfo);
    }
}
