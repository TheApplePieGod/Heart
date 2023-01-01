#include "hepch.h"
#include "ImGuiUtils.h"

#include "Heart/Container/HVector.hpp"
#include "Heart/Container/HString.h"
#include "Heart/Asset/AssetManager.h"
#include "imgui/imgui_internal.h"

namespace Heart
{
    void ImGuiUtils::RenderTooltip(const HStringView8& text)
    {
        if (ImGui::IsItemHovered())
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 5.f, 5.f} );
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(text.Data());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
            ImGui::PopStyleVar();
        }
    }

    bool ImGuiUtils::InputText(const char* id, HString8& text)
    {
        char buffer[128];
        memset(buffer, 0, sizeof(buffer));
        std::strncpy(buffer, text.Data(), sizeof(buffer));
        if (ImGui::InputText(id, buffer, sizeof(buffer)))
        {
            ImGui::SetKeyboardFocusHere(-1);
            text = buffer;
            return true;
        }
        return false;
    }

    bool ImGuiUtils::InputText(const char* id, HString& text)
    {
        char buffer[128];
        memset(buffer, 0, sizeof(buffer));
        std::strncpy(buffer, text.DataUTF8(), sizeof(buffer));
        if (ImGui::InputText(id, buffer, sizeof(buffer)))
        {
            ImGui::SetKeyboardFocusHere(-1);
            text = buffer;
            return true;
        }
        return false;
    }

    void ImGuiUtils::DrawFilterPopup(const char* popupName, bool focusOnOpen, std::function<void()>&& drawCallback, std::function<void()>&& clearCallback)
    {
        ImGui::SameLine();

        HString8 b1id = HStringView8("V##v") + HStringView8(popupName);
        HString8 b2id = HStringView8("X##x") + HStringView8(popupName);

        bool popupOpened = ImGui::Button(b1id.Data());
        if (popupOpened)
            ImGui::OpenPopup(popupName);

        ImGui::SameLine(0.f, 3.f);
        if (ImGui::Button(b2id.Data()))
            clearCallback();

        if (ImGui::BeginPopup(popupName, ImGuiWindowFlags_HorizontalScrollbar))
        {
            drawCallback();
            if (focusOnOpen && popupOpened)
                ImGui::SetKeyboardFocusHere(-1);
            ImGui::EndPopup();
        }
    }

    void ImGuiUtils::DrawTextFilter(ImGuiTextFilter& filter, const char* popupName)
    {
        DrawFilterPopup(
            popupName,
            true,
            [&filter] ()
            {
                if (filter.Draw())
                    ImGui::SetKeyboardFocusHere(-1);
            },
            [&filter] ()
            {
                filter.Clear();
            }
        );
    }

    void ImGuiUtils::DrawStringDropdownFilter(const char** options, u32 optionCount, u32& selected, const char* popupName)
    {
        ImGuiUtils::DrawFilterPopup(
            popupName,
            false,
            [&selected, options, optionCount] ()
            {
                for (u32 i = 0; i < optionCount; i++)
                    if (ImGui::Selectable(options[i], selected == i))
                        selected = i;
            },
            [&selected] ()
            {
                selected = 0;
            }
        );
    }

    void ImGuiUtils::AssetDropTarget(Asset::Type typeFilter, std::function<void(const HStringView8&)>&& dropCallback)
    {
        if (AssetManager::GetAssetsDirectory().IsEmpty());
            return;

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FileTransfer"))
            {
                const char* payloadData = (const char*)payload->Data;
                HString8 relativePath = std::filesystem::path(payloadData).lexically_relative(AssetManager::GetAssetsDirectory().Data()).generic_u8string();
                auto assetType = AssetManager::DeduceAssetTypeFromFile(relativePath);

                if ((typeFilter == Asset::Type::None || assetType == typeFilter) && dropCallback)
                    dropCallback(relativePath);
            }
            ImGui::EndDragDropTarget();
        }
    }

    bool ImGuiUtils::XYZSlider(HStringView8 name, f32* x, f32* y, f32* z, f32 min, f32 max, f32 step)
    {
        bool modified = false;
        f32 width = ImGui::GetContentRegionAvail().x;
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.f, 0.5f));
        if (ImGui::BeginTable(name.Data(), 7, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextRow();

            ImU32 xColor = ImGui::GetColorU32(ImVec4(1.f, 0.0f, 0.0f, 1.f));
            ImU32 yColor = ImGui::GetColorU32(ImVec4(0.f, 1.0f, 0.0f, 1.f));
            ImU32 zColor = ImGui::GetColorU32(ImVec4(0.f, 0.0f, 1.0f, 1.f));
            ImU32 textColor = ImGui::GetColorU32(ImVec4(0.f, 0.0f, 0.0f, 1.f));

            ImGui::TableSetColumnIndex(0);
            ImGui::Text(name.Data());

            ImGui::TableSetColumnIndex(1);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, xColor);
            ImGui::Text(" X ");
            
            ImGui::TableSetColumnIndex(2);
            ImGui::PushItemWidth(width * 0.15f);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, textColor);
            modified |= ImGui::DragFloat("##x", x, step, min, max, "%.2f");

            ImGui::TableSetColumnIndex(3);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, yColor);
            ImGui::Text(" Y ");
            
            ImGui::TableSetColumnIndex(4);
            ImGui::PushItemWidth(width * 0.15f);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, textColor);
            modified |= ImGui::DragFloat("##y", y, step, min, max, "%.2f");

            ImGui::TableSetColumnIndex(5);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, zColor);
            ImGui::Text(" Z ");
            
            ImGui::TableSetColumnIndex(6);
            ImGui::PushItemWidth(width * 0.15f);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, textColor);
            modified |= ImGui::DragFloat("##z", z, step, min, max, "%.2f");

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        
        return modified;
    }

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

    void ImGuiUtils::AssetPicker(
        Asset::Type assetType,
        UUID selectedAsset,
        const HStringView8& nullSelectionText,
        const HStringView8& widgetId,
        ImGuiTextFilter& textFilter,
        std::function<void()>&& contextMenuCallback,
        std::function<void(UUID)>&& selectCallback
    )
    {
        const auto& UUIDRegistry = AssetManager::GetUUIDRegistry();

        bool valid = selectedAsset && UUIDRegistry.find(selectedAsset) != UUIDRegistry.end();

        HString8 buttonNullSelection = nullSelectionText + HStringView8("##") + widgetId;
        HString8 popupName = widgetId + HStringView8("AP");
        bool popupOpened = ImGui::Button(valid ? UUIDRegistry.at(selectedAsset).Path.Data() : buttonNullSelection.Data());
        if (popupOpened)
            ImGui::OpenPopup(popupName.Data());
        
        // right click menu
        if (contextMenuCallback && ImGui::BeginPopupContextItem((widgetId + HStringView8("ctx")).Data()))
        {
            contextMenuCallback();
            ImGui::EndPopup();
        }

        ImGui::SetNextWindowSize({ 500.f, std::min(ImGui::GetMainViewport()->Size.y - ImGui::GetCursorScreenPos().y, 500.f) });
        if (ImGui::BeginPopup(popupName.Data(), ImGuiWindowFlags_HorizontalScrollbar))
        {
            if (textFilter.Draw() || popupOpened)
                ImGui::SetKeyboardFocusHere(-1);
            ImGui::Separator();
            for (auto& pair : UUIDRegistry)
            {
                if (pair.second.Type == assetType && textFilter.PassFilter(pair.second.Path.Data()))
                {
                    if (ImGui::MenuItem(pair.second.Path.Data()))
                    {
                        selectCallback(pair.first);
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
            ImGui::EndPopup();
        }
    }
    
    void ImGuiUtils::StringPicker(
        const HVector<HString8>& options,
        const HStringView8& selected,
        const HStringView8& nullSelectionText,
        const HStringView8& widgetId,
        ImGuiTextFilter& textFilter,
        std::function<void()>&& contextMenuCallback,
        std::function<void(u32)>&& selectCallback
    )
    {
        HString8 buttonNullSelection = nullSelectionText + HStringView8("##") + widgetId;
        HString8 popupName = widgetId + HStringView8("SP");
        bool popupOpened = ImGui::Button(selected.IsEmpty() ? buttonNullSelection.Data() : selected.Data());
        if (popupOpened)
            ImGui::OpenPopup(popupName.Data());
        
        // right click menu
        if (contextMenuCallback && ImGui::BeginPopupContextItem((widgetId + HStringView8("ctx")).Data()))
        {
            contextMenuCallback();
            ImGui::EndPopup();
        }

        ImGui::SetNextWindowSize({ 500.f, std::min(ImGui::GetMainViewport()->Size.y - ImGui::GetCursorScreenPos().y, 500.f) });
        if (ImGui::BeginPopup(popupName.Data(), ImGuiWindowFlags_HorizontalScrollbar))
        {
            if (textFilter.Draw() || popupOpened)
                ImGui::SetKeyboardFocusHere(-1);
            ImGui::Separator();
            for (u32 i = 0; i < options.Count(); i++)
            {
                if (textFilter.PassFilter(options[i].Data()) && ImGui::MenuItem(options[i].Data()))
                {
                    selectCallback(i);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }
    }
}