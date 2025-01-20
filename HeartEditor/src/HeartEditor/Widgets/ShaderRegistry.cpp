#include "Heart/Util/BitUtils.hpp"
#include "hepch.h"
#include "ShaderRegistry.h"

#include "HeartEditor/Editor.h"
#include "Heart/Container/HString8.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Util/ImGuiUtils.h"
#include "Heart/Asset/ShaderAsset.h"

namespace HeartEditor
{
namespace Widgets
{
    void ShaderRegistry::OnImGuiRenderPostSceneUpdate()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Open) return;

        static const std::array<const char*, 9> shaderTypeStrs = {
            "None",
            "Vertex", "Fragment", "Compute", "RayGen",
            "RayMiss", "RayIntersection", "RayClosestHit", "RayAnyHit"
        };

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.Data(), &m_Open);

        const int columnCount = 3;
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2.f, 2.f));
        if (ImGui::BeginTable("RegistryTable", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableNextColumn();
            ImGui::Text("Path");
            Heart::ImGuiUtils::DrawTextFilter(m_PathFilter, "##pathfilter");

            ImGui::TableNextColumn();
            ImGui::Text("Type");
            Heart::ImGuiUtils::DrawStringDropdownFilter(
                shaderTypeStrs.data(),
                shaderTypeStrs.size(),
                m_ShaderTypeFilter,
                "##typefilter"
            );

            ImGui::TableNextColumn();
            ImGui::Text("Reload");

            auto& registry = Heart::AssetManager::GetUUIDRegistry();
            Heart::HVector<Heart::ShaderAsset*> filteredRegistry;
            for (auto& pair : registry)
            {
                if (pair.second.Type != Heart::Asset::Type::Shader)
                    continue;

                Heart::ShaderAsset* asset = Heart::AssetManager::RetrieveAsset<Heart::ShaderAsset>(pair.first);
                if (asset->IsValid() &&
                    PassShaderTypeFilter((u32)asset->GetShader()->GetType()) &&
                    m_PathFilter.PassFilter(pair.second.Path.Data())
                )
                    filteredRegistry.Add(asset);
            }

            ImGuiListClipper clipper; // For virtualizing the list
            clipper.Begin(filteredRegistry.Count());
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    auto asset = filteredRegistry[i];

                    ImGui::TableNextRow();

                    Heart::HString8 id1 = Heart::HStringView8("##") + Heart::HStringView8(asset->GetPath());
                    Heart::HString8 id2 = id1 + "p";

                    ImGui::TableNextColumn();
                    ImGui::BeginDisabled();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::InputText(id2.Data(), (char*)asset->GetPath().Data(), asset->GetPath().Count(), ImGuiInputTextFlags_ReadOnly);
                    ImGui::EndDisabled();

                    u32 typeIdx = asset->GetShader()->GetType();
                    if (typeIdx > 0)
                        typeIdx = Heart::BitUtils::PowerOfTwoIndex(asset->GetShader()->GetType()) + 1;
                    if (typeIdx >= shaderTypeStrs.size())
                        typeIdx = 0;
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", shaderTypeStrs[typeIdx]);

                    Heart::HString8 buttonName = Heart::HStringView8("Reload") + Heart::HStringView8(id1);
                    ImGui::TableNextColumn();
                    if (ImGui::Button(buttonName.Data()))
                    {
                        // TODO: async?
                        bool success = asset->GetShader()->Reload();
                        
                        StatusElement status;
                        status.Duration = 4000.f;
                        status.Type = success ? StatusElementType::Success : StatusElementType::Error;
                        status.Text = success ? "Shader reload successful!" : "Shader reload failed, check logs";
                        Editor::PushStatus(status);
                    }
                }
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        

        ImGui::End();
        ImGui::PopStyleVar();
    }

    bool ShaderRegistry::PassShaderTypeFilter(u32 type)
    {
        return m_ShaderTypeFilter == 0 ||
               m_ShaderTypeFilter == type;
    }
}
}
