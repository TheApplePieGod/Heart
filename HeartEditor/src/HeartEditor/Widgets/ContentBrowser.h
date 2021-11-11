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

        void InitializeTextureReistry();
        void OnImGuiRender();
        void ShutdownTextureRegistry() { m_CBTextures.reset(); }

    private:
        void ScanDirectory();
        void RenderFileCard(const std::filesystem::directory_entry& entry);
        
    private:
        const glm::vec2 m_CardSize = { 75.f, 75.f };
        const f32 m_CardSpacing = 5.f;
        glm::vec2 m_WindowSizes = { 200.f, 5000.f };
        Heart::Scope<Heart::TextureRegistry> m_CBTextures;
        std::string m_DefaultAssetDirectory = "assets";
        std::vector<std::filesystem::directory_entry> m_DirectoryList;
        bool m_ShouldRescan = false;
    };
}
}