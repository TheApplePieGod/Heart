#include "htpch.h"
#include "MeshAsset.h"

#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Asset/AssetManager.h"
#include "nlohmann/json.hpp"

namespace Heart
{
    void MeshAsset::Load()
    {
        if (m_Loaded) return;

        u32 fileLength;
        unsigned char* data = FilesystemUtils::ReadFile(m_Path, fileLength);

        if (/*m_Extension == ".glb" ||*/ m_Extension == ".gltf") // TODO: glb support
            ParseGLTF(data);
        else
        { HE_ENGINE_ASSERT(false, "Unsupported mesh type"); }

        delete[] data;
        m_Loaded = true;
    }

    void MeshAsset::Unload()
    {
        if (!m_Loaded) return;

        m_Submeshes.clear();
        m_DefaultTexturePaths.clear();
        m_Loaded = false;
    }

    void MeshAsset::ParseGLTF(unsigned char* data)
    {
        auto j = nlohmann::json::parse(data);
        
        // parse buffers
        std::vector<std::vector<unsigned char>> buffers;
        for (auto& buffer : j["buffers"])
        {
            std::string uri = buffer["uri"];
            if (uri.find("base64") != std::string::npos)
            {
                std::string base64 = uri.substr(uri.find(',') + 1);
                buffers.emplace_back(Base64Decode(base64));
            }
            else if (uri.find(".bin") != std::string::npos)
            {
                u32 fileLength;
                std::string binPath = std::filesystem::path(m_Path).parent_path().append(uri).generic_u8string();
                unsigned char* bin = FilesystemUtils::ReadFile(binPath, fileLength);
                buffers.emplace_back();
                buffers.back().assign(bin, bin + fileLength);
            }
            else
            { HE_ENGINE_ASSERT(false, "Buffer data is not in a supported format"); }
        }

        // parse buffer views
        std::vector<BufferView> bufferViews;
        for (auto& view : j["bufferViews"])
        {
            bufferViews.emplace_back(view["buffer"], view["byteLength"], view.contains("byteOffset") ? view["byteOffset"] : 0);
        }

        // parse accessors
        std::vector<Accessor> accessors;
        for (auto& accessor : j["accessors"])
        {
            u32 byteOffset = accessor.contains("byteOffset") ? accessor["byteOffset"] : 0;
            accessors.emplace_back(accessor["bufferView"], byteOffset, accessor["count"], accessor["componentType"]);
        }

        // parse texture sources
        auto parentPath = std::filesystem::path(m_Path).parent_path();
        std::vector<std::string> texturePaths;
        if (j.contains("images"))
        {
            for (auto& image : j["images"])
            {
                std::string uri = image["uri"];
                if (uri.find("base64") != std::string::npos)
                {
                    HE_ENGINE_ASSERT(false, "Loading inline textures is not supported yet");
                    //std::string base64 = uri.substr(uri.find(',') + 1);
                    //buffers.emplace_back(Base64Decode(base64));
                }
                else
                {
                    auto finalPath = parentPath.append(uri).generic_u8string();
                    texturePaths.emplace_back(finalPath);
                    AssetManager::RegisterAsset(Asset::Type::Texture, finalPath);
                    parentPath = parentPath.parent_path();
                }
            }
        }

        // parse textures
        std::vector<TextureView> textureViews;
        if (j.contains("textures"))
        {
            for (auto& texture : j["textures"])
            {
                textureViews.emplace_back(texture["sampler"], texture["source"]);
            }
        }

        // parse materials
        if (j.contains("materials"))
        {
            for (auto& material : j["materials"])
            {
                // TODO: color materials
                if (material["pbrMetallicRoughness"].contains("baseColorTexture"))
                {
                    u32 texIndex = material["pbrMetallicRoughness"]["baseColorTexture"]["index"];
                    m_DefaultTexturePaths.emplace_back(texturePaths[textureViews[texIndex].SourceIndex]);
                }
                else
                    m_DefaultTexturePaths.emplace_back("");
            }
        }
        else
            m_DefaultTexturePaths.emplace_back(""); // should always have one material

        // parse meshes and create submeshes based on material
        std::unordered_map<u32, SubmeshParseData> parseData; // keyed by material id
        for (auto& mesh : j["meshes"])
        {
            for (auto& primitive : mesh["primitives"])
            {
                u32 materialIndex = primitive.contains("material") ? primitive["material"] : 0;
                if (parseData.find(materialIndex) == parseData.end())
                    parseData[materialIndex] = SubmeshParseData();

                auto& smData = parseData[materialIndex];

                // Points = 0, Lines, LineLoop, LineStrip, Triangles, TriangleStrip, TriangleFan
                if (primitive.contains("mode"))
                { HE_ENGINE_ASSERT(primitive["mode"] == 4, "Cannot load GLTF mesh that uses an unsupported primitive mode"); } // TODO: support all modes
                HE_ENGINE_ASSERT(primitive.contains("indices"), "Cannot load GLTF mesh that does not use indexed geometry");

                // parse indices
                {
                    auto& accessor = accessors[primitive["indices"]];
                    auto& bufferView = bufferViews[accessor.BufferViewIndex];
                    auto& buffer = buffers[bufferView.BufferIndex];

                    if (accessor.Count + smData.IndexOffset > smData.Indices.size())
                        smData.Indices.resize(accessor.Count + smData.Indices.size());

                    u32 offset = 0; // bytes
                    for (u32 i = smData.IndexOffset; i < smData.IndexOffset + accessor.Count; i++)
                    {
                        if (accessor.ComponentType == 5123) // unsigned short
                        {
                            smData.Indices[i] = *(u16*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]) + smData.VertexOffset;
                            offset += 2;
                        }
                        else if (accessor.ComponentType == 5125) // unsigned int
                        {
                            smData.Indices[i] = *(u32*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]) + smData.VertexOffset;
                            offset += 4;
                        }
                        else
                        { HE_ENGINE_ASSERT(false, "Cannot load GLTF mesh that has indices with an unsupported data type"); }
                    }

                    smData.IndexOffset += accessor.Count;
                }

                u32 lastCount = 0;
                for (auto& attribute : primitive["attributes"].items())
                {
                    auto& accessor = accessors[attribute.value()];
                    auto& bufferView = bufferViews[accessor.BufferViewIndex];
                    auto& buffer = buffers[bufferView.BufferIndex];

                    if (accessor.Count + smData.VertexOffset > smData.Vertices.size())
                        smData.Vertices.resize(accessor.Count + smData.Vertices.size());

                    // parse vertex data
                    u32 offset = 0; // bytes
                    for (u32 i = smData.VertexOffset; i < smData.VertexOffset + accessor.Count; i++)
                    {
                        if (attribute.key() == "POSITION")
                        {
                            smData.Vertices[i].Position = *(glm::vec3*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]);
                            offset += 12;
                        }
                        else if (attribute.key() == "TEXCOORD_0")
                        {
                            smData.Vertices[i].UV = *(glm::vec2*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]);
                            offset += 8;
                        }
                        else if (attribute.key() == "NORMAL")
                        {
                            smData.Vertices[i].Normal = *(glm::vec3*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]);
                            offset += 12;
                        }
                        else if (attribute.key() == "TANGENT")
                        {
                            smData.Vertices[i].Tangent = *(glm::vec4*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]);
                            offset += 16;
                        }
                    }

                    lastCount = accessor.Count;
                }
                smData.VertexOffset += lastCount;
            }
        }

        // populate submeshes with parsed data
        for (auto& pair : parseData)
            m_Submeshes.emplace_back(pair.second.Vertices, pair.second.Indices, pair.first); 
    }

    // adapted from https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
    std::vector<unsigned char> MeshAsset::Base64Decode(const std::string& encoded)
    {
        int in_len = static_cast<int>(encoded.size());
        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<unsigned char> ret;

        while (in_len-- && ( encoded[in_] != '=') && IsBase64(encoded[in_]))
        {
            char_array_4[i++] = encoded[in_]; in_++;
            if (i ==4)
            {
                for (i = 0; i <4; i++)
                    char_array_4[i] = static_cast<unsigned char>(s_Base64Chars.find(char_array_4[i]));

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; (i < 3); i++)
                    ret.push_back(char_array_3[i]);
                i = 0;
            }
        }

        if (i)
        {
            for (j = i; j <4; j++)
                char_array_4[j] = 0;

            for (j = 0; j <4; j++)
                char_array_4[j] = static_cast<unsigned char>(s_Base64Chars.find(char_array_4[j]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (j = 0; (j < i - 1); j++)
                ret.push_back(char_array_3[j]);
        }

        return ret;
    }
}