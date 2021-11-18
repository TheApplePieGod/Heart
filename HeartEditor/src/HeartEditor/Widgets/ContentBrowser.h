#pragma once

#include "Heart/Renderer/Texture.h"
#include "glm/vec2.hpp"

namespace HeartEditor
{
namespace Widgets
{
    class ContentBrowser
    {
    public:
        ContentBrowser();

        void OnImGuiRender();

    private:
        void ScanDirectory();
        void RenderFileCard(const std::filesystem::directory_entry& entry);
        void RenderDirectoryNode(const std::string& path);
        void PushDirectoryStack(const std::string& entry);

    private:
        const glm::vec2 m_CardSize = { 75.f, 75.f };
        const f32 m_CardSpacing = 5.f;
        glm::vec2 m_WindowSizes = { 200.f, 5000.f };
        std::vector<std::string> m_DirectoryStack = { "assets" };
        int m_DirectoryStackIndex = 0;
        std::vector<std::filesystem::directory_entry> m_DirectoryList;
        bool m_ShouldRescan = false;
    };
}
}