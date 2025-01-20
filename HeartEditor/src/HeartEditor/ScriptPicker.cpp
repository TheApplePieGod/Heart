#include "Heart/Scripting/ScriptingEngine.h"
#include "hepch.h"
#include "ScriptPicker.h"

#include "HeartEditor/Editor.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Util/ImGuiUtils.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace HeartEditor
{
    void ScriptPicker::OnImGuiRender(
        s64 currentScript,
        Heart::HStringView8 selectText,
        std::function<void()>&& contextMenuCallback,
        std::function<void(s64)>&& selectCallback
    ) {
        HE_PROFILE_FUNCTION();
        
        auto& classDict = Heart::ScriptingEngine::GetEntityClasses();

        m_Processing.Clear();
        for (auto& pair : classDict)
        {
            HE_ENGINE_ASSERT(pair.second.GetFullName().GetEncoding() == Heart::HString::Encoding::UTF8);

            if (!m_NameFilter.PassFilter(pair.second.GetFullName().DataUTF8()))
                continue;

            m_Processing.Add({ pair.first, pair.second.GetFullName() });
        }

        Heart::HStringView8 buttonText = currentScript == 0
            ? selectText
            : classDict.count(currentScript) > 0
                ? classDict.at(currentScript).GetFullName().DataUTF8()
                : "<Invalid>";
        m_Picker.OnImGuiRender(
            buttonText,
            "Select Script",
            m_Processing.Count(), 1,
            [this]()
            {
                ImGui::Text("Name");
                Heart::ImGuiUtils::DrawTextFilter(m_NameFilter, "##namefilter");
            },
            [this](u32 index)
            {
                auto& pair = m_Processing[index];

                ImGui::Text("%s", pair.second.DataUTF8());
            },
            std::move(contextMenuCallback),
            [this, selectCallback](u32 index)
            {
                selectCallback(m_Processing[index].first);
            }
        );
    }
}
