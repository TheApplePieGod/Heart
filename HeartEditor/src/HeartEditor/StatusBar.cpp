#include "hepch.h"
#include "StatusBar.h"

#include "HeartEditor/Editor.h"
#include "HeartEditor/EditorApp.h"
#include "Heart/ImGui/ImGuiInstance.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Util/ImGuiUtils.h"
#include "imgui/imgui.h"

namespace HeartEditor
{
    void StatusBar::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();
        
        Editor::LockStatus();

        auto& elements = Editor::GetState().StatusElements;

        const f32 elementHeight = 50.f;
        const f32 elementWidth = 400.f;

        ImVec2 windowSize = ImGui::GetWindowSize();
        
        ImGui::SetCursorPos({ 10.f, windowSize.y - 10.f - elementHeight * elements.Count() });
        ImGui::BeginChild("##statusbar");

        Heart::HString8 vertId = "v";
        for (u32 i = 0; i < elements.Count(); i++)
        {
            auto& elem = elements[i];

            // Scale opacity based on duration left
            const float fadeStartTime = 500.f;
            float opacity = elem.Duration > fadeStartTime ? 1.f : 1.f - (fadeStartTime - std::max(elem.Duration, 0.f)) / fadeStartTime;

            ImGui::SetCursorPos({ 0.f, elementHeight * i });
            ImVec2 start = ImGui::GetCursorScreenPos();
            ImVec2 end = { start.x + elementWidth, start.y + elementHeight };
            ImGui::GetWindowDrawList()->AddRectFilled(start, end, IM_COL32( 50, 50, 50, 128 * opacity )); // viewport background
            ImGui::BeginHorizontal((vertId + std::to_string(i)).Data(), { elementWidth, elementHeight }, 0.5f);

            Heart::HStringView8 texture;
            switch (elem.Type)
            {
                case StatusElementType::Info: { texture = "editor/info.png"; } break;
                case StatusElementType::Warn: { texture = "editor/warning.png"; } break;
                case StatusElementType::Error: { texture = "editor/error.png"; } break;
                case StatusElementType::Success: { texture = "editor/success.png"; } break;
                case StatusElementType::Loading: { texture = "editor/loading.png"; } break;
            }

            ImGui::Dummy({ 6.f, 0.f });

            ImGui::Image(
                Heart::AssetManager::RetrieveAsset(Heart::HString8(texture), true)
                    ->EnsureValid<Heart::TextureAsset>()
                    ->GetTexture()
                    ->GetImGuiHandle(),
                { elementHeight * 0.5f, elementHeight * 0.5f },
                { 0, 0 }, { 1, 1 },
                { 1, 1, 1, opacity }
            );

            ImGui::PushFont(EditorApp::Get().GetImGuiInstance().GetLargeFont());
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255 * opacity));
            ImGui::Text("%s", elem.Text.Data());
            ImGui::PopStyleColor();
            ImGui::PopFont();

            ImGui::EndHorizontal();
        }

        ImGui::EndChild();

        Editor::UnlockStatus();
    }
}
