#include "hepch.h"
#include "SplatAsset.h"

#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Util/StringUtils.hpp"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Asset/TextureAsset.h"
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

        SplatData splatData;
        HVector<SplatData> splatDatas;
        float* floatData = (float*)data;
        for (u32 i = 0; i < vertexCount; i++)
        {
            //1499000

            glm::vec3 scale = {
                floatData[8 * i + 3 + 0],
                floatData[8 * i + 3 + 1],
                floatData[8 * i + 3 + 2],
            };
            glm::mat4 scaleMat = glm::scale(glm::mat4(1.f), scale);

            glm::vec4 pos = {
                floatData[8 * i + 0],
                floatData[8 * i + 1],
                floatData[8 * i + 2],
                1.f
            };

            glm::quat quat = {
                ((float)data[32 * i + 28 + 0] - 128.f) / 128.f,
                ((float)data[32 * i + 28 + 1] - 128.f) / 128.f,
                ((float)data[32 * i + 28 + 2] - 128.f) / 128.f,
                ((float)data[32 * i + 28 + 3] - 128.f) / 128.f,
            };
            glm::mat4 rotMat = glm::toMat4(quat);

            splatData.Position = pos;
            splatData.Color = {
                data[32 * i + 24 + 0] / 255.f,
                data[32 * i + 24 + 1] / 255.f,
                data[32 * i + 24 + 2] / 255.f,
                data[32 * i + 24 + 3] / 255.f
            };
            // TODO: this is symmetric, so we can save on storage space
            splatData.Sigma = {
                rotMat * scaleMat * glm::transpose(scaleMat) * glm::transpose(rotMat)
            };

            splatDatas.AddInPlace(splatData);

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

            //if (i > 500000)
            //    break;
        }

        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Type = Flourish::BufferType::Storage;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Static;
        bufCreateInfo.Stride = sizeof(SplatData);
        bufCreateInfo.ElementCount = splatDatas.Count();
        bufCreateInfo.InitialData = splatDatas.Data();
        bufCreateInfo.InitialDataSize = sizeof(SplatData) * splatDatas.Count();
        m_DataBuffer = Flourish::Buffer::Create(bufCreateInfo);
    }
}
