#include "htpch.h"
#include "MeshAsset.h"

#include "Heart/Util/FilesystemUtils.h"
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
            bufferViews.emplace_back(view["buffer"], view["byteLength"], view["byteOffset"]);
        }

        // parse accessors
        std::vector<Accessor> accessors;
        for (auto& accessor : j["accessors"])
        {
            u32 byteOffset = accessor.contains("byteOffset") ? accessor["byteOffset"] : 0;
            accessors.emplace_back(accessor["bufferView"], byteOffset, accessor["count"], accessor["componentType"]);
        }

        // parse meshes
        std::vector<Mesh::Vertex> vertices;
        std::vector<u32> indices;
        for (auto& mesh : j["meshes"])
        {
            vertices.clear();
            indices.clear();
            
            u32 primitiveVertexOffset = 0;
            u32 primitiveIndexOffset = 0;
            for (auto& primitive : mesh["primitives"])
            {
                // Points = 0, Lines, LineLoop, LineStrip, Triangles, TriangleStrip, TriangleFan
                if (primitive.contains("mode"))
                { HE_ENGINE_ASSERT(primitive["mode"] == 4, "Cannot load GLTF mesh that uses an unsupported primitive mode"); } // TODO: support all modes
                HE_ENGINE_ASSERT(primitive.contains("indices"), "Cannot load GLTF mesh that does not use indexed geometry");

                u32 materialIndex = primitive.contains("material") ? primitive["material"] : 0;

                // parse indices
                {
                    auto& accessor = accessors[primitive["indices"]];
                    auto& bufferView = bufferViews[accessor.BufferViewIndex];
                    auto& buffer = buffers[bufferView.BufferIndex];

                    if (accessor.Count + primitiveIndexOffset > indices.size())
                        indices.resize(accessor.Count + indices.size());

                    u32 offset = 0; // bytes
                    for (u32 i = primitiveIndexOffset; i < primitiveIndexOffset + accessor.Count; i++)
                    {
                        if (accessor.ComponentType == 5123) // unsigned short
                        {
                            indices[i] = *(u16*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]) + primitiveVertexOffset;
                            offset += 2;
                        }
                        else if (accessor.ComponentType == 5125) // unsigned int
                        {
                            indices[i] = *(u32*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]) + primitiveVertexOffset;
                            offset += 4;
                        }
                        else
                        { HE_ENGINE_ASSERT(false, "Cannot load GLTF mesh that has indices with an unsupported data type"); }
                    }

                    primitiveIndexOffset += accessor.Count;
                }

                u32 lastCount = 0;
                for (auto& attribute : primitive["attributes"].items())
                {
                    auto& accessor = accessors[attribute.value()];
                    auto& bufferView = bufferViews[accessor.BufferViewIndex];
                    auto& buffer = buffers[bufferView.BufferIndex];

                    if (accessor.Count + primitiveVertexOffset > vertices.size())
                        vertices.resize(accessor.Count + vertices.size());

                    // parse vertex data
                    u32 offset = 0; // bytes
                    for (u32 i = primitiveVertexOffset; i < primitiveVertexOffset + accessor.Count; i++)
                    {
                        if (attribute.key() == "POSITION")
                        {
                            vertices[i].Position = *(glm::vec3*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]);
                            offset += 12;
                        }
                        else if (attribute.key() == "TEXCOORD_0")
                        {
                            vertices[i].UV = *(glm::vec2*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]);
                            offset += 8;
                        }
                        else if (attribute.key() == "NORMAL")
                        {
                            vertices[i].Normal = *(glm::vec3*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]);
                            offset += 12;
                        }
                        else if (attribute.key() == "TANGENT")
                        {
                            vertices[i].Tangent = *(glm::vec4*)(&buffer[bufferView.ByteOffset + accessor.ByteOffset + offset]);
                            offset += 16;
                        }
                        vertices[i].MaterialIndex = materialIndex;
                    }

                    lastCount = accessor.Count;
                }
                primitiveVertexOffset += lastCount;
            }

            m_Submeshes.emplace_back(mesh.contains("name") ? mesh["name"] : "Mesh", vertices, indices); 
        }
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