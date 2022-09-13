#include "hepch.h"
#include "Toolbar.h"

#include "HeartEditor/Editor.h"
#include "Heart/Renderer/Texture.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Util/ImGuiUtils.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
    void Toolbar::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();
        
        float maxWidth = ImGui::GetContentRegionAvail().x;
        float buttonSize = 35.f;
        float spacing = 0.f;
        float numButtons = 1.f;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0.f));
        ImGui::SetCursorPosX(maxWidth * 0.5f - buttonSize * 0.5f - spacing * (numButtons - 1));
        if (ImGui::ImageButton(
            Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(
                Editor::GetSceneState() == SceneState::Editing ? "editor/play.png" : "editor/stop.png", true
            )->GetTexture()->GetImGuiHandle(),
            { 35, 35 }
        ))
        {
            if (Editor::GetSceneState() == SceneState::Editing)
                Editor::PlayScene();
            else
                Editor::StopScene();
        }
        ImGui::PopStyleVar();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 1.f));
        ImGui::Separator();
        ImGui::PopStyleVar();
    }
}