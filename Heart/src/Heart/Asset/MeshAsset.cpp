#include "htpch.h"
#include "MeshAsset.h"

#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Asset/TextureAsset.h"
#include "nlohmann/json.hpp"

namespace Heart
{
    void MeshAsset::Load()
    {
        if (m_Loaded) return;

        u32 fileLength;
        unsigned char* data = nullptr;

        try
        {
            data = FilesystemUtils::ReadFile(m_AbsolutePath, fileLength);
            if (/*m_Extension == ".glb" ||*/ m_Extension == ".gltf") // TODO: glb support
                ParseGLTF(data);
        }
        catch (std::exception e)
        {
            HE_ENGINE_LOG_ERROR("Failed to load mesh at path {0}", m_AbsolutePath);
            m_Loaded = true;
            return;
        }

        delete[] data;
        m_Loaded = true;
        m_Valid = true;
    }

    void MeshAsset::Unload()
    {
        if (!m_Loaded) return;

        m_Submeshes.clear();
        m_DefaultMaterials.clear();
        m_Loaded = false;
        m_Valid = false;
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
                std::string binPath = std::filesystem::path(m_AbsolutePath).parent_path().append(uri).generic_u8string();
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
        std::vector<TextureSource> textureSources;
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
                    std::string finalPath = std::filesystem::path(m_ParentPath).append(uri).generic_u8string();
                    textureSources.emplace_back(finalPath, AssetManager::RegisterAsset(Asset::Type::Texture, finalPath));
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
        std::string materialFilenameStart = "material";
        std::string materialFilenameEnd = ".hemat";
        if (j.contains("materials"))
        {
            u32 materialIndex = 0;
            for (auto& material : j["materials"])
            {
                auto& pbrField = material["pbrMetallicRoughness"];
                Material parsingMaterial;

                if (pbrField.contains("baseColorFactor"))
                    parsingMaterial.m_MaterialData.BaseColor = { pbrField["baseColorFactor"][0], pbrField["baseColorFactor"][1], pbrField["baseColorFactor"][2], pbrField["baseColorFactor"][3] };
                if (pbrField.contains("baseColorTexture"))
                {
                    u32 texIndex = material["pbrMetallicRoughness"]["baseColorTexture"]["index"];
                    parsingMaterial.m_AlbedoTextureAsset = textureSources[textureViews[texIndex].SourceIndex].AssetId;

                    // TODO: change this possibly
                    auto texAsset = AssetManager::RetrieveAsset<TextureAsset>(parsingMaterial.m_AlbedoTextureAsset);
                    if (texAsset && texAsset->IsValid())
                        parsingMaterial.m_Transparent = texAsset->GetTexture()->HasTransparency();
                }

                std::string localPath = std::filesystem::path(m_ParentPath).append(materialFilenameStart + std::to_string(materialIndex) + materialFilenameEnd).generic_u8string();
                std::string finalPath = std::filesystem::path(AssetManager::GetAssetsDirectory()).append(localPath).generic_u8string();
                MaterialAsset::SerializeMaterial(finalPath, parsingMaterial);
                m_DefaultMaterials.emplace_back(AssetManager::RegisterAsset(Asset::Type::Material, localPath));
                materialIndex++;
            }
        }
        else // should always have one material
        {
            Material genericMaterial;
            std::string localPath = std::filesystem::path(m_ParentPath).append(materialFilenameStart + "0" + materialFilenameEnd).generic_u8string();
            std::string finalPath = std::filesystem::path(AssetManager::GetAssetsDirectory()).append(localPath).generic_u8string();
            MaterialAsset::SerializeMaterial(finalPath, genericMaterial);
            m_DefaultMaterials.emplace_back(AssetManager::RegisterAsset(Asset::Type::Material, finalPath));
        }

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
}