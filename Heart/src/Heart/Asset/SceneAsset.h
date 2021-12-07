#pragma once

#include "Heart/Asset/Asset.h"

namespace Heart
{
    class Scene;
    class SceneAsset : public Asset
    {
    public:
        /**
         * @brief Default constructor.
         * 
         * @param path The path of the asset relative to the project directory.
         * @param absolutePath The absolute filesystem path of the asset.
         */
        SceneAsset(const std::string& path, const std::string& absolutePath)
            : Asset(path, absolutePath)
        { m_Type = Type::Scene; }

        void Load() override;
        void Unload() override;

        /*! @brief Take the current loaded scene and serialize it to the underlying asset file. */
        void SaveChanges() { SerializeScene(m_AbsolutePath, m_Scene.get()); }

        /*! @brief Get a shared reference to the scene stored in this asset. */
        Ref<Scene> GetScene() { return m_Scene; }

    public:
        /**
         * @brief Load a scene from disk.
         * 
         * @param path The absolute path of the scene file. 
         * @return A shared reference containing the loaded scene.
         */
        static Ref<Scene> DeserializeScene(const std::string& path);

        /**
         * @brief Save a scene to disk.
         * 
         * @param path The absolute path of the output file.
         * @param scene The scene to serialize.
         */
        static void SerializeScene(const std::string& path, Scene* scene);

    private:
        Ref<Scene> m_Scene;
    };
}