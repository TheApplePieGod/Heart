#pragma once

#include "Heart/Asset/Asset.h"

namespace Heart
{
    class Scene;
    class SceneAsset : public Asset
    {
    public:
        SceneAsset(const std::string& path, const std::string& absolutePath)
            : Asset(path, absolutePath)
        { m_Type = Type::Scene; }

        void Load() override;
        void Unload() override;
        void SaveChanges() { SerializeScene(m_AbsolutePath, m_Scene.get()); }

        Ref<Scene> GetScene() { return m_Scene; }

    public:
        static Ref<Scene> DeserializeScene(const std::string& path);
        static void SerializeScene(const std::string& path, Scene* scene);

    private:
        Ref<Scene> m_Scene;
    };
}