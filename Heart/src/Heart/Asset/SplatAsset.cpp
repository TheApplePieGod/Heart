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

        float eps = 1e-10f;
        float max = 99999.f;

        SplatData splatData;
        HVector<SplatData> splatDatas;
        float* floatData = (float*)data;
        for (u32 i = 0; i < vertexCount; i++)
        {
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

            if (glm::length(glm::vec3(pos)) < 0.001f)
                continue;

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
            glm::mat4 M = rotMat * scaleMat;
            glm::mat4 sigma = M * glm::transpose(M);

            // Ensure sigma is valid, skip otherwise
            if (abs(sigma[0][0]) <= eps || abs(sigma[0][0]) >= max || std::isnan(sigma[0][0]) ||
                abs(sigma[0][1]) <= eps || abs(sigma[0][1]) >= max || std::isnan(sigma[0][1]) ||
                abs(sigma[1][1]) <= eps || abs(sigma[1][1]) >= max || std::isnan(sigma[1][1]) ||
                abs(sigma[0][2]) <= eps || abs(sigma[0][2]) >= max || std::isnan(sigma[0][2]) ||
                abs(sigma[1][2]) <= eps || abs(sigma[1][2]) >= max || std::isnan(sigma[1][2]) ||
                abs(sigma[2][2]) <= eps || abs(sigma[2][2]) >= max || std::isnan(sigma[2][2]))
                continue;

            // Pack sigma since the matrix is symmetric
            splatData.PackedSigma = {
                PackFloats(sigma[0][0], sigma[0][1]),
                PackFloats(sigma[0][2], sigma[1][1]),
                PackFloats(sigma[1][2], sigma[2][2]),
                0
            };

            splatDatas.AddInPlace(splatData);

            /*
            HE_LOG_WARN(
                "X: {0}, Y: {1}, Z: {2}",
                sigma[0][0],
                sigma[1][1],
                sigma[2][2]
            );

            HE_LOG_WARN(
                "R: {0}, G: {1}, B: {2}, A: {3}",
                colors.Back().r,
                colors.Back().g,
                colors.Back().b,
                colors.Back().a
            );
            */

            /*
            if (i > 50000)
                break;
                */
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

    u32 SplatAsset::PackFloats(float a, float b)
    {
        return ((u32)FloatToHalf(b) << 16) | (u32)FloatToHalf(a);
    }

    u16 SplatAsset::FloatToHalf(float a)
    {
        // https://stackoverflow.com/questions/1659440/32-bit-to-16-bit-floating-point-conversion
        u32 casted = *(u32*)&a;
        const u32 b = casted + 0x00001000;
        const u32 e = (b & 0x7F800000) >> 23;
        const u32 m = b & 0x007FFFFF;
        return (b & 0x80000000) >> 16 |
               (e > 112) * ((((e - 112) << 10) & 0x7C00) | m >> 13) |
               ((e < 113) & (e > 101)) * ((((0x007FF000 + m) >> (125 - e)) + 1) >> 1) |
               (e > 143) * 0x7FFF;
    }
}
