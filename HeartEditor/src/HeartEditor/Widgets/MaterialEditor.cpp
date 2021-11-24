#include "htpch.h"
#include "MaterialEditor.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Scene/Components.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Input/Input.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "Heart/Util/ImGuiUtils.h"

namespace HeartEditor
{
namespace Widgets
{
    MaterialEditor::MaterialEditor()
    {
        m_Scene = Heart::CreateRef<Heart::Scene>();

        m_DemoEntity = m_Scene->CreateEntity("Demo Entity");
        m_DemoEntity.AddComponent<Heart::MeshComponent>();
        m_DemoEntity.GetComponent<Heart::MeshComponent>().Mesh = Heart::AssetManager::GetAssetUUID("DefaultCube.gltf", true);
        m_DemoEntity.GetComponent<Heart::MeshComponent>().Materials.push_back(0);

        m_SceneCamera.UpdateViewMatrix({ 0.f, 0.f, 0.f }, m_Radius, 0, 0);
        m_SceneCameraPosition = -m_SceneCamera.GetForwardVector() * m_Radius;
    }

    void MaterialEditor::Initialize()
    {
        m_SceneRenderer = Heart::CreateScope<Heart::SceneRenderer>();
    }

    void MaterialEditor::Shutdown()
    {
        m_SceneRenderer.reset();
        m_FirstRender = true;
    }

    void MaterialEditor::OnImGuiRender(Heart::EnvironmentMap* envMap, Heart::UUID selectedMaterial, bool* dirty)
    {
        HE_PROFILE_FUNCTION();
        
        auto materialAsset = Heart::AssetManager::RetrieveAsset<Heart::MaterialAsset>(selectedMaterial);
        bool shouldRender = materialAsset && materialAsset->IsValid();

        if ((shouldRender && ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) || m_FirstRender || selectedMaterial != m_LastMaterial)
        {
            if (selectedMaterial != m_LastMaterial)
            {
                *dirty = false;
                m_CachedMaterial = materialAsset->GetMaterial();
            }

            m_LastMaterial = selectedMaterial;
            m_FirstRender = false;
            m_DemoEntity.GetComponent<Heart::MeshComponent>().Materials[0] = selectedMaterial;
            m_SceneCamera.UpdateAspectRatio(m_WindowSizes.y / ImGui::GetContentRegionMax().y); // update aspect using estimated size
            m_SceneRenderer->RenderScene(
                EditorApp::Get().GetWindow().GetContext(),
                m_Scene.get(),
                m_SceneCamera,
                m_SceneCameraPosition,
                envMap
            );
        }

        Heart::ImGuiUtils::ResizableWindowSplitter(
            m_WindowSizes,
            { 100.f, 100.f },
            true,
            6.f,
            10.f,
            !ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows),
            [&]() { RenderSidebar(selectedMaterial, dirty); },
            [&]() { RenderViewport(shouldRender); }
        );
    }

    void MaterialEditor::RenderSidebar(Heart::UUID selectedMaterial, bool* dirty)
    {
        if (!selectedMaterial)
        {
            ImGui::TextColored({ 1.f, 1.f, 0.f, 1.f }, "No Material Selected");
            return;
        }

        auto materialAsset = Heart::AssetManager::RetrieveAsset<Heart::MaterialAsset>(selectedMaterial);
        if (materialAsset && materialAsset->IsValid())
        {
            auto& material = materialAsset->GetMaterial();
            auto& materialData = material.GetMaterialData();

            if (*dirty)
                ImGui::BeginDisabled();
            if (ImGui::Button("Save Changes"))
            {
                *dirty = false;
                materialAsset->SaveChanges();
                m_CachedMaterial = material;
            }
            ImGui::SameLine();
            if (ImGui::Button("Revert Changes"))
            {
                *dirty = false;
                material = m_CachedMaterial;
            }
            ImGui::Dummy({ 0.f, 5.f });
            if (*dirty)
                ImGui::EndDisabled();

            ImGui::Text("Material Properties");
            ImGui::Separator();

            // careful here, we are editing the material data properties directly so if the layout changes we need to come back in here and fix them
            ImGui::Text("Base Color");
            ImGui::SameLine();
            if (ImGui::ColorEdit4("##Base Color", (float*)&materialData.BaseColor, ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_AlphaBar))
                *dirty = true;
            ImGui::Text("Emissive Factor");
            ImGui::SameLine();
            if (ImGui::ColorEdit3("##Emissive Factor", (float*)&materialData.EmissiveFactor))
                *dirty = true;
            ImGui::Text("Metalness");
            ImGui::SameLine();
            if (ImGui::DragFloat("##Metalness", &materialData.Scalars.x, 0.05f, 0.f, 1.f))
                *dirty = true;
            ImGui::Text("Roughness");
            ImGui::SameLine();
            if (ImGui::DragFloat("##Roughness", &materialData.Scalars.y, 0.05f, 0.f, 1.f))
                *dirty = true;

            float previewSize = 128.f;
            ImGui::Dummy({ 0.f, 5.f });
            ImGui::Text("Material Textures");
            ImGui::Separator();

            // ---------------------------------
            // Albedo select
            // ---------------------------------
            ImGui::Text("Albedo:");
            ImGui::SameLine();
            Heart::ImGuiUtils::AssetPicker(
                Heart::Asset::Type::Texture,
                material.GetAlbedoTexture(),
                "NULL",
                "AlbedoSelect",
                m_TextureTextFilter,
                [&material, dirty]()
                {
                    if (ImGui::MenuItem("Clear"))
                    {
                        material.SetAlbedoTexture(0);
                        *dirty = true;
                    }
                },
                [&material, dirty](Heart::UUID selected) { material.SetAlbedoTexture(selected); *dirty = true; }
            );
            auto albedoAsset = Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(material.GetAlbedoTexture());
            if (albedoAsset && albedoAsset->IsValid())
                ImGui::Image(albedoAsset->GetTexture()->GetImGuiHandle(0), { previewSize, previewSize });

            ImGui::Separator();

            // ---------------------------------
            // MetallicRoughness select
            // ---------------------------------
            ImGui::Text("Metallic Roughness:");
            ImGui::SameLine();
            Heart::ImGuiUtils::AssetPicker(
                Heart::Asset::Type::Texture,
                material.GetMetallicRoughnessTexture(),
                "NULL",
                "MRSelect",
                m_TextureTextFilter,
                [&material, dirty]()
                {
                    if (ImGui::MenuItem("Clear"))
                    {
                        material.SetMetallicRoughnessTexture(0);
                        *dirty = true;
                    }
                },
                [&material, dirty](Heart::UUID selected) { material.SetMetallicRoughnessTexture(selected); *dirty = true; }
            );
            auto mrAsset = Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(material.GetMetallicRoughnessTexture());
            if (mrAsset && mrAsset->IsValid())
                ImGui::Image(mrAsset->GetTexture()->GetImGuiHandle(0), { previewSize, previewSize });

            ImGui::Separator();

            // ---------------------------------
            // Normal select
            // ---------------------------------
            ImGui::Text("Normal:");
            ImGui::SameLine();
            Heart::ImGuiUtils::AssetPicker(
                Heart::Asset::Type::Texture,
                material.GetNormalTexture(),
                "NULL",
                "NormalSelect",
                m_TextureTextFilter,
                [&material, dirty]()
                {
                    if (ImGui::MenuItem("Clear"))
                    {
                        material.SetNormalTexture(0);
                        *dirty = true;
                    }
                },
                [&material, dirty](Heart::UUID selected) { material.SetNormalTexture(selected); *dirty = true; }
            );
            auto normalAsset = Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(material.GetNormalTexture());
            if (normalAsset && normalAsset->IsValid())
                ImGui::Image(normalAsset->GetTexture()->GetImGuiHandle(0), { previewSize, previewSize });

            ImGui::Separator();

            // ---------------------------------
            // Emissive select
            // ---------------------------------
            ImGui::Text("Emissive:");
            ImGui::SameLine();
            Heart::ImGuiUtils::AssetPicker(
                Heart::Asset::Type::Texture,
                material.GetEmissiveTexture(),
                "NULL",
                "EmissiveSelect",
                m_TextureTextFilter,
                [&material, dirty]()
                {
                    if (ImGui::MenuItem("Clear"))
                    {
                        material.SetEmissiveTexture(0);
                        *dirty = true;
                    }
                },
                [&material, dirty](Heart::UUID selected) { material.SetEmissiveTexture(selected); *dirty = true; }
            );
            auto emAsset = Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(material.GetEmissiveTexture());
            if (emAsset && emAsset->IsValid())
                ImGui::Image(emAsset->GetTexture()->GetImGuiHandle(0), { previewSize, previewSize });

            ImGui::Separator();

            // ---------------------------------
            // Occlusion select
            // ---------------------------------
            ImGui::Text("Occlusion:");
            ImGui::SameLine();
            Heart::ImGuiUtils::AssetPicker(
                Heart::Asset::Type::Texture,
                material.GetOcclusionTexture(),
                "NULL",
                "OCSelect",
                m_TextureTextFilter,
                [&material, dirty]()
                {
                    if (ImGui::MenuItem("Clear"))
                    {
                        material.SetOcclusionTexture(0);
                        *dirty = true;
                    }
                },
                [&material, dirty](Heart::UUID selected) { material.SetOcclusionTexture(selected); *dirty = true; }
            );
            auto ocAsset = Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(material.GetOcclusionTexture());
            if (ocAsset && ocAsset->IsValid())
                ImGui::Image(ocAsset->GetTexture()->GetImGuiHandle(0), { previewSize, previewSize });
        }
        else
            ImGui::TextColored({ 0.0f, 0.1f, 0.1f, 1.f }, "Invalid Material");
    }

    void MaterialEditor::RenderViewport(bool shouldRender)
    {
        if (!shouldRender) return;

        // calculate viewport bounds
        ImVec2 viewportMin = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMax = ImGui::GetWindowContentRegionMax();
        ImVec2 viewportPos = ImGui::GetWindowPos();
        glm::vec2 viewportSize = { viewportMax.x - viewportMin.x, viewportMax.y - viewportMin.y };
        glm::vec2 viewportStart = { viewportMin.x + viewportPos.x, viewportMin.y + viewportPos.y };
        glm::vec2 viewportEnd = viewportStart + viewportSize;

        // draw the viewport background
        ImGui::GetWindowDrawList()->AddRectFilled({ viewportStart.x, viewportStart.y }, { viewportEnd.x, viewportEnd.y }, IM_COL32( 0, 0, 0, 255 )); // viewport background

        // draw the rendered texture
        ImGui::Image(
            m_SceneRenderer->GetFinalFramebuffer().GetColorAttachmentImGuiHandle(0),
            { viewportSize.x, viewportSize.y }
        );
        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseDragging(0))
            {
                m_SwivelRotation.x += static_cast<float>(Heart::Input::GetMouseDeltaX());
                m_SwivelRotation.y += static_cast<float>(Heart::Input::GetMouseDeltaY());
            }
            m_Radius = std::clamp(m_Radius + static_cast<float>(-Heart::Input::GetScrollOffsetY()), 0.f, 100.f);
            m_SceneCamera.UpdateViewMatrix({ 0.f, 0.f, 0.f }, m_Radius, m_SwivelRotation.x, m_SwivelRotation.y);
            m_SceneCameraPosition = -m_SceneCamera.GetForwardVector() * m_Radius;
        }
    }
}
}