#include "htpch.h"
#include "SceneAsset.h"

#include "Heart/Util/FilesystemUtils.h"
#include "Heart/Asset/AssetManager.h"
#include "nlohmann/json.hpp"
#include "Heart/Scene/Entity.h"

namespace Heart
{
    void SceneAsset::Load()
    {
        if (m_Loaded || m_Loading) return;
        m_Loading = true;

        try
        {
            m_Scene = DeserializeScene(m_AbsolutePath);
        }
        catch (std::exception e)
        {
            HE_ENGINE_LOG_ERROR("Failed to load scene at path {0}", m_AbsolutePath);
            m_Loaded = true;
            m_Loading = false;
            return;
        }

        m_Data = nullptr;
        m_Loaded = true;
        m_Loading = false;
        m_Valid = true;
    }

    void SceneAsset::Unload()
    {
        if (!m_Loaded) return;

        m_Scene.reset();

        m_Data = nullptr;
        m_Loaded = false;
        m_Valid = false;
    }

    Ref<Scene> SceneAsset::DeserializeScene(const std::string& path)
    {
        auto scene = CreateRef<Scene>();


        //delete[] data;
        return scene;
    }

    void SceneAsset::SerializeScene(const std::string& path, Scene* scene)
    {
        nlohmann::json j;

        // entities
        {
            auto& field = j["entities"];
            
            u32 index = 0;
            scene->GetRegistry().each([scene, &index, &field](auto handle)
            {
                nlohmann::json entry;
                Entity entity = { scene, handle };
                
                // Id component
                entry["idComponent"]["id"] = static_cast<u64>(entity.GetUUID());

                // Name component
                entry["nameComponent"]["name"] = entity.GetName();

                // Parent component
                if (entity.HasComponent<ParentComponent>())
                    entry["parentComponent"]["id"] = static_cast<u64>(entity.GetComponent<ParentComponent>().ParentUUID);

                // Child component
                if (entity.HasComponent<ChildComponent>())
                {
                    auto& childComp = entity.GetComponent<ChildComponent>();
                    for (size_t i = 0; i < childComp.Children.size(); i++)
                        entry["childComponent"]["children"][i] = static_cast<u64>(childComp.Children[i]);
                }

                // Transform component
                auto& transformComp = entity.GetComponent<TransformComponent>();
                entry["transformComponent"]["translation"] = nlohmann::json::array({ transformComp.Translation.x, transformComp.Translation.y, transformComp.Translation.z });
                entry["transformComponent"]["rotation"] = nlohmann::json::array({ transformComp.Rotation.x, transformComp.Rotation.y, transformComp.Rotation.z });
                entry["transformComponent"]["scale"] = nlohmann::json::array({ transformComp.Scale.x, transformComp.Scale.y, transformComp.Scale.z });

                // Mesh component
                if (entity.HasComponent<MeshComponent>())
                {
                    auto& meshComp = entity.GetComponent<MeshComponent>();
                    entry["meshComponent"]["mesh"] = AssetManager::GetPathFromUUID(meshComp.Mesh);
                    for (size_t i = 0; i < meshComp.Materials.size(); i++)
                        entry["meshComponent"]["materials"][i] = AssetManager::GetPathFromUUID(meshComp.Materials[i]);
                }

                field[index++] = entry;
            });
        }

        // settings
        {
            auto& field = j["settings"];
        }

        std::ofstream file(path);
        file << j;
    }
}