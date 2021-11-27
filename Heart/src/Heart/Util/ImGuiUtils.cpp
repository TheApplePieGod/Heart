#include "htpch.h"
#include "ImGuiUtils.h"

#include "Heart/Asset/AssetManager.h"
#include "imgui/imgui_internal.h"

namespace Heart
{
    void ImGuiUtils::ResizableWindowSplitter(glm::vec2& storedWindowSizes, glm::vec2 minWindowSize, bool isHorizontal, float splitterThickness, float windowSpacing, bool splitterDisable, std::function<void()>&& window1Contents, std::function<void()>&& window2Contents)
    {
        f32 availableWidth = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
        f32 availableHeight = ImGui::GetContentRegionAvail().y;

        // update the 'max size'
        if (storedWindowSizes.x == 0 && storedWindowSizes.y == 0)
        {
            if (isHorizontal)
                storedWindowSizes = { 500, 500 };
            else
                storedWindowSizes = { availableHeight * 0.5f, availableHeight * 0.5f };
        }

        if (isHorizontal)
            storedWindowSizes.y = availableWidth - storedWindowSizes.x - windowSpacing - splitterThickness;
        else
            storedWindowSizes.y = availableHeight - storedWindowSizes.x - windowSpacing - splitterThickness;

        // draw the splitter that allows for child window resizing
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiID id = window->GetID("##Splitter");
        ImRect bb;
        bb.Min = ImVec2(window->DC.CursorPos.x + (isHorizontal ? storedWindowSizes.x : 0.f), window->DC.CursorPos.y + (isHorizontal ? 0.f : storedWindowSizes.x));
        bb.Max = ImGui::CalcItemSize(isHorizontal ? ImVec2(splitterThickness, availableHeight) : ImVec2(availableWidth, splitterThickness), 0.0f, 0.0f);
        bb.Max = { bb.Max.x + bb.Min.x, bb.Max.y + bb.Min.y };

        if (splitterDisable)
            ImGui::BeginDisabled();
        ImGui::SplitterBehavior(bb, id, isHorizontal ? ImGuiAxis_X : ImGuiAxis_Y, &storedWindowSizes.x, &storedWindowSizes.y, minWindowSize.x, minWindowSize.y, 0.0f);
        if (splitterDisable)
            ImGui::EndDisabled();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::BeginChild("splitterChild1", ImVec2(isHorizontal ? storedWindowSizes.x : availableWidth, isHorizontal ? availableHeight : storedWindowSizes.x), false);
        
        window1Contents();

        ImGui::EndChild();
        
        if (isHorizontal)
            ImGui::SameLine(0.f, windowSpacing);
        else
            ImGui::Dummy({ 0.f, windowSpacing });            

        ImGui::BeginChild("splitterChild2", ImVec2(isHorizontal ? storedWindowSizes.y : availableWidth, isHorizontal ? availableHeight : storedWindowSizes.y), false);
        
        window2Contents();

        ImGui::EndChild();
        ImGui::PopStyleVar();
    }

    void ImGuiUtils::AssetPicker(Asset::Type assetType, UUID selectedAsset, const std::string& nullSelectionText, const std::string& widgetId, ImGuiTextFilter& textFilter, std::function<void()>&& contextMenuCallback, std::function<void(UUID)>&& selectCallback)
    {
        const auto& UUIDRegistry = AssetManager::GetUUIDRegistry();

        bool valid = selectedAsset && UUIDRegistry.find(selectedAsset) != UUIDRegistry.end();

        std::string buttonNullSelection = nullSelectionText + "##" + widgetId;
        std::string popupName = widgetId + "SP";
        bool popupOpened = ImGui::Button(valid ? UUIDRegistry.at(selectedAsset).Path.c_str() : buttonNullSelection.c_str());
        if (popupOpened)
            ImGui::OpenPopup(popupName.c_str());
        
        // right click menu
        if (contextMenuCallback && ImGui::BeginPopupContextItem((widgetId + "context").c_str()))
        {
            contextMenuCallback();
            ImGui::EndPopup();
        }

        ImGui::SetNextWindowSize({ 500.f, std::min(ImGui::GetMainViewport()->Size.y - ImGui::GetCursorScreenPos().y, 500.f) });
        if (ImGui::BeginPopup(popupName.c_str(), ImGuiWindowFlags_HorizontalScrollbar))
        {
            if (textFilter.Draw() || popupOpened)
                ImGui::SetKeyboardFocusHere(-1);
            ImGui::Separator();
            for (auto& pair : UUIDRegistry)
            {
                if (pair.second.Type == assetType && textFilter.PassFilter(pair.second.Path.c_str()))
                {
                    if (ImGui::MenuItem(pair.second.Path.c_str()))
                    {
                        selectCallback(pair.first);
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            ImGui::EndPopup();
        }
    }
}