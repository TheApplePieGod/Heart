#include "hepch.h"
#include "MaterialEditor.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Core/Window.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Renderer/SceneRenderer.h"
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
    MaterialEditor::MaterialEditor(const std::string& name, bool initialOpen)
        : Widget(name, initialOpen)
    {
        m_SceneRenderer = Heart::CreateScope<Heart::SceneRenderer>();
        m_Scene = Heart::CreateRef<Heart::Scene>();
        m_Scene->SetEnvironmentMap(Heart::AssetManager::GetAssetUUID("engine/DefaultEnvironmentMap.hdr", true));

        m_DemoEntity = m_Scene->CreateEntity("Demo Entity");
        m_DemoEntity.AddComponent<Heart::MeshComponent>();
        m_DemoEntity.GetComponent<Heart::MeshComponent>().Mesh = Heart::AssetManager::GetAssetUUID("engine/DefaultCube.gltf", true);
        m_DemoEntity.GetComponent<Heart::MeshComponent>().Materials.push_back(0);

        m_SceneCamera.UpdateViewMatrix({ 0.f, 0.f, 0.f }, m_Radius, 0, 0);
        m_SceneCameraPosition = -m_SceneCamera.GetForwardVector() * m_Radius;
    }

    void MaterialEditor::Reset()
    {
        m_FirstRender = true;
    }

    void MaterialEditor::OnImGuiRender()
    {
        HE_PROFILE_FUNCTION();

        // Open the window when a new material is selected
        if (m_SelectedMaterial != m_LastMaterial)
            m_Open = true;

        if (!m_Open) return;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.c_str(), &m_Open);
        
        auto materialAsset = Heart::AssetManager::RetrieveAsset<Heart::MaterialAsset>(m_SelectedMaterial);
        bool shouldRender = materialAsset && materialAsset->IsValid();

        // Only render the window when:
        //  - Valid material & window is focused
        //  - The material was just changed
        //  - Graphics api was just initialized (first render)
        if ((shouldRender && ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) || m_FirstRender || m_SelectedMaterial != m_LastMaterial)
        {
            // Cache the material for reverting changes later
            if (m_SelectedMaterial != m_LastMaterial)
            {
                m_Dirty = false;
                m_CachedMaterial = materialAsset->GetMaterial();
            }

            Heart::SceneRenderSettings renderSettings;
            renderSettings.DrawGrid = false;
            renderSettings.CullEnable = false;

            // Updates
            m_LastMaterial = m_SelectedMaterial;
            m_FirstRender = false;
            m_DemoEntity.GetComponent<Heart::MeshComponent>().Materials[0] = m_SelectedMaterial;
            m_SceneCamera.UpdateAspectRatio(m_WindowSizes.y / ImGui::GetContentRegionMax().y); // update aspect using estimated size
            m_SceneRenderer->RenderScene(
                EditorApp::Get().GetWindow().GetContext(),
                m_Scene.get(),
                m_SceneCamera,
                m_SceneCameraPosition,
                renderSettings
            );
        }

        Heart::ImGuiUtils::ResizableWindowSplitter(
            m_WindowSizes,
            { 100.f, 100.f },
            true,
            6.f,
            10.f,
            !ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows),
            [&]() { RenderSidebar(); },
            [&]() { RenderViewport(shouldRender); }
        );

        // Display material on drop
        Heart::ImGuiUtils::AssetDropTarget(
            Heart::Asset::Type::Material,
            [&](const std::string& path) { m_SelectedMaterial = Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Material, path); }
        );

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void MaterialEditor::RenderSidebar()
    {
        if (!m_SelectedMaterial)
        {
            ImGui::TextColored({ 1.f, 1.f, 0.f, 1.f }, "No Material Selected");
            return;
        }

        auto materialAsset = Heart::AssetManager::RetrieveAsset<Heart::MaterialAsset>(m_SelectedMaterial);
        if (materialAsset && materialAsset->IsValid())
        {
            // Do not allow modification of engine resources, but still allow for them to be displayed
            if (Heart::AssetManager::IsAssetAnEngineResource(m_SelectedMaterial))
            {
                ImGui::TextColored({ 1.f, 1.f, 0.f, 1.f }, "Cannot Modify an Engine Resource");
                return;
            }

            auto& material = materialAsset->GetMaterial();
            auto& materialData = material.GetMaterialData();

            // Material path
            ImGui::Text(materialAsset->GetPath().c_str());

            // Save buttons
            bool disabled = !m_Dirty;
            if (disabled)
                ImGui::BeginDisabled();
            if (ImGui::Button("Save Changes"))
            {
                m_Dirty = false;
                materialAsset->SaveChanges();
                m_CachedMaterial = material;
            }
            ImGui::SameLine();
            if (ImGui::Button("Revert Changes"))
            {
                m_Dirty = false;
                material = m_CachedMaterial;
            }
            ImGui::Dummy({ 0.f, 5.f });
            if (disabled)
                ImGui::EndDisabled();

            ImGui::Text("Material Properties");
            ImGui::Separator();

            // Careful here, we are editing the material data properties directly so if the layout changes we need to come back in here and fix them
            ImGui::Text("Base Color");
            ImGui::SameLine();
            if (ImGui::ColorEdit4("##Base Color", (float*)&materialData.BaseColor, ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_AlphaBar))
                m_Dirty = true;
            ImGui::Text("Emissive Factor");
            ImGui::SameLine();
            if (ImGui::ColorEdit3("##Emissive Factor", (float*)&materialData.EmissiveFactor))
                m_Dirty = true;
            ImGui::Text("Metalness");
            ImGui::SameLine();
            if (ImGui::DragFloat("##Metalness", &materialData.Scalars.x, 0.05f, 0.f, 1.f))
                m_Dirty = true;
            ImGui::Text("Roughness");
            ImGui::SameLine();
            if (ImGui::DragFloat("##Roughness", &materialData.Scalars.y, 0.05f, 0.f, 1.f))
                m_Dirty = true;
            ImGui::Text("Alpha Clip Threshold");
            ImGui::SameLine();
            if (ImGui::DragFloat("##ACThreshold", &materialData.Scalars.z, 0.01f, 0.f, 1.f))
                m_Dirty = true;
            bool isTranslucent = material.IsTranslucent();
            ImGui::Text("Is Translucent");
            ImGui::SameLine();
            if (ImGui::Checkbox("##Translucent", &isTranslucent))
            {
                material.SetTranslucent(isTranslucent);
                m_Dirty = true;
            }

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
                [&]()
                {
                    if (ImGui::MenuItem("Clear"))
                    {
                        material.SetAlbedoTexture(0);
                        m_Dirty = true;
                    }
                },
                [&](Heart::UUID selected) { material.SetAlbedoTexture(selected); m_Dirty = true; }
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
                [&]()
                {
                    if (ImGui::MenuItem("Clear"))
                    {
                        material.SetMetallicRoughnessTexture(0);
                        m_Dirty = true;
                    }
                },
                [&](Heart::UUID selected) { material.SetMetallicRoughnessTexture(selected); m_Dirty = true; }
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
                [&]()
                {
                    if (ImGui::MenuItem("Clear"))
                    {
                        material.SetNormalTexture(0);
                        m_Dirty = true;
                    }
                },
                [&](Heart::UUID selected) { material.SetNormalTexture(selected); m_Dirty = true; }
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
                [&]()
                {
                    if (ImGui::MenuItem("Clear"))
                    {
                        material.SetEmissiveTexture(0);
                        m_Dirty = true;
                    }
                },
                [&](Heart::UUID selected) { material.SetEmissiveTexture(selected); m_Dirty = true; }
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
                [&]()
                {
                    if (ImGui::MenuItem("Clear"))
                    {
                        material.SetOcclusionTexture(0);
                        m_Dirty = true;
                    }
                },
                [&](Heart::UUID selected) { material.SetOcclusionTexture(selected); m_Dirty = true; }
            );
            auto ocAsset = Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(material.GetOcclusionTexture());
            if (ocAsset && ocAsset->IsValid())
                ImGui::Image(ocAsset->GetTexture()->GetImGuiHandle(0), { previewSize, previewSize });
        }
        else
            ImGui::TextColored({ 0.9f, 0.1f, 0.1f, 1.f }, "Invalid Material");
    }

    void MaterialEditor::RenderViewport(bool shouldRender)
    {
        if (!shouldRender) return;

        // Calculate viewport bounds
        ImVec2 viewportMin = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMax = ImGui::GetWindowContentRegionMax();
        ImVec2 viewportPos = ImGui::GetWindowPos();
        glm::vec2 viewportSize = { viewportMax.x - viewportMin.x, viewportMax.y - viewportMin.y };
        glm::vec2 viewportStart = { viewportMin.x + viewportPos.x, viewportMin.y + viewportPos.y };
        glm::vec2 viewportEnd = viewportStart + viewportSize;

        // Draw the viewport background
        ImGui::GetWindowDrawList()->AddRectFilled({ viewportStart.x, viewportStart.y }, { viewportEnd.x, viewportEnd.y }, IM_COL32( 0, 0, 0, 255 )); // viewport background

        // Draw the rendered texture
        ImGui::Image(
            //m_SceneRenderer->GetFinalFramebuffer().GetColorAttachmentImGuiHandle(0),
            m_SceneRenderer->GetFinalTexture().GetImGuiHandle(),
            { viewportSize.x, viewportSize.y }
        );

        // If we are dragging in the viewport, update the swivel camera rotation
        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseDragging(0))
            {
                m_SwivelRotation.x += static_cast<float>(Heart::Input::GetMouseDeltaX());
                m_SwivelRotation.y += static_cast<float>(Heart::Input::GetMouseDeltaY());
            }

            // Camera will orbit around { 0, 0, 0 }
            m_Radius = std::clamp(m_Radius + static_cast<float>(-Heart::Input::GetScrollOffsetY()), 0.f, 100.f);
            m_SceneCamera.UpdateViewMatrix({ 0.f, 0.f, 0.f }, m_Radius, m_SwivelRotation.x, m_SwivelRotation.y);
            m_SceneCameraPosition = -m_SceneCamera.GetForwardVector() * m_Radius;
        }
    }
}
}