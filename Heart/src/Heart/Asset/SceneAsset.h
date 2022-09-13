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
        SceneAsset(const HStringView8& path, const HStringView8& absolutePath)
            : Asset(path, absolutePath)
        { m_Type = Type::Scene; }

        void Load(bool async = false) override;
        void Unload() override;

        /*! @brief Take a scene and serialize it to the underlying asset file. */
        void Save(Scene* scene);

        /*! @brief Get a shared reference to the scene stored in this asset. */
        inline Ref<Scene> GetScene() const { return m_Scene; }

    public:
        /**
         * @brief Load a scene from disk.
         * 
         * @param path The absolute path of the scene file. 
         * @return A shared reference containing the loaded scene.
         */
        static Ref<Scene> DeserializeScene(const HStringView8& path);

        /**
         * @brief Save a scene to disk.
         * 
         * @param path The absolute path of the output file.
         * @param scene The scene to serialize.
         */
        static void SerializeScene(const HStringView8& path, Scene* scene);

    private:
        Ref<Scene> m_Scene;
    };
}