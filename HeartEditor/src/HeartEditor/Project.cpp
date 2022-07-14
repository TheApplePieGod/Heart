#include "hepch.h"
#include "Project.h"

#include "HeartEditor/Editor.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Util/FilesystemUtils.h"
#include "nlohmann/json.hpp"

namespace HeartEditor
{
    Heart::Ref<Project> Project::s_ActiveProject = nullptr;

    Heart::Ref<Project> Project::CreateAndLoad(const std::string& absolutePath, const std::string& name)
    {
        const char* extension = ".heproj";
        std::string filename = name + extension;

        std::filesystem::path directoryPath = std::filesystem::path(absolutePath).append(name);
        std::filesystem::create_directory(directoryPath);

        std::filesystem::path finalPath = directoryPath.append(filename);
        
        nlohmann::json j;
        j["name"] = name;

        std::ofstream file(finalPath);
        file << j;

        file.close();
        return LoadFromPath(finalPath.generic_u8string());
    }

    Heart::Ref<Project> Project::LoadFromPath(const std::string& absolutePath)
    {
        Heart::Ref<Project> project = Heart::CreateRef<Project>(absolutePath);
        
        u32 fileLength;
        unsigned char* data = Heart::FilesystemUtils::ReadFile(absolutePath, fileLength);
        if (!data)
            throw std::exception();

        auto j = nlohmann::json::parse(data);
        project->m_Name = j["name"];

        // Unload active scene (load empty)
        Editor::SetActiveScene(Heart::CreateRef<Heart::Scene>());

        Heart::AssetManager::UpdateAssetsDirectory(
            Heart::FilesystemUtils::GetParentDirectory(absolutePath)
        );

        s_ActiveProject = project;
        delete[] data;
        return project;
    }
}