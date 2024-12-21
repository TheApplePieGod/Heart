#include "hepch.h"
#include "MaterialEditor.h"

#include "HeartEditor/EditorApp.h"
#include "Heart/Core/Window.h"
#include "Flourish/Api/Context.h"
#include "Heart/Renderer/DesktopSceneRenderer.h"
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
    MaterialEditor::MaterialEditor(const Heart::HStringView8& name, bool initialOpen)
        : Widget(name, initialOpen)
    {
        m_SceneRenderer = Heart::CreateScope<Heart::DesktopSceneRenderer>(false);
        m_Scene = Heart::CreateRef<Heart::Scene>();
        m_Scene->SetEnvironmentMap(Heart::AssetManager::GetAssetUUID("engine/DefaultEnvironmentMap.hdr", true));

        m_DemoEntity = m_Scene->CreateEntity("Demo Entity");
        m_DemoEntity.AddComponent<Heart::MeshComponent>();
        m_DemoEntity.GetComponent<Heart::MeshComponent>().Mesh = Heart::AssetManager::GetAssetUUID("engine/DefaultCube.gltf", true);
        m_DemoEntity.GetComponent<Heart::MeshComponent>().Materials.Add(0);

        m_SceneCamera.UpdateViewMatrix(glm::vec3(0.f), m_Radius, glm::vec3(0.f));
        m_SceneCameraPosition = -m_SceneCamera.GetForwardVector() * m_Radius;

        Reset();
    }

    MaterialEditor::~MaterialEditor()
    {
        Heart::AssetManager::UnregisterAsset(m_EditingMaterialAsset);
    }

    void MaterialEditor::OnImGuiRenderPostSceneUpdate()
    {
        HE_PROFILE_FUNCTION();

        // Open the window when a new material is selected
        if (m_SelectedMaterial != m_LastMaterial)
            m_Open = true;

        if (!m_Open)
        {
            m_LastMaterial = 0;
            m_SelectedMaterial = 0;
            return;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
        ImGui::Begin(m_Name.Data(), &m_Open);
        
        auto materialAsset = Heart::AssetManager::RetrieveAsset<Heart::MaterialAsset>(m_SelectedMaterial);
        bool shouldRenderViewport = materialAsset && materialAsset->Load()->IsValid();
        bool materialChanged = m_SelectedMaterial != m_LastMaterial; 
        bool shouldRerender = (shouldRenderViewport && ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
            || materialChanged || m_RenderedFrames < Flourish::Context::FrameBufferCount();

        // Only render the window when:
        //  - Valid material & window is focused
        //  - The material was just changed
        //  - We have framebuffers to fill (i.e. double/triple buffering)
        if (shouldRerender)
        {
            // Update the editing material with the new material value
            if (materialChanged)
            {
                m_Dirty = false;
                m_RenderedFrames = 0;

                // Will always be valid & loaded (in-memory)
                auto editingAsset = Heart::AssetManager::RetrieveAsset<Heart::MaterialAsset>(m_EditingMaterialAsset);
                editingAsset->GetMaterial() = materialAsset->GetMaterial();
            }

            m_RenderedFrames++;

            Heart::SceneRenderSettings renderSettings;
            renderSettings.DrawGrid = false;
            renderSettings.CullEnable = false;

            // Updates
            m_LastMaterial = m_SelectedMaterial;
            m_DemoEntity.GetComponent<Heart::MeshComponent>().Materials[0] = m_EditingMaterialAsset;
            m_SceneCamera.UpdateAspectRatio(m_WindowSizes.y / ImGui::GetContentRegionMax().y); // update aspect using estimated size
            m_RenderScene.CopyFromScene(m_Scene.get());
            m_SceneRenderer->Render({
                &m_RenderScene,
                m_Scene->GetEnvironmentMap(),
                &m_SceneCamera,
                m_SceneCameraPosition,
                renderSettings
            }).Wait();

            Flourish::Context::PushFrameRenderGraph(m_SceneRenderer->GetRenderGraph());
        }

        Heart::ImGuiUtils::ResizableWindowSplitter(
            m_WindowSizes,
            { 100.f, 100.f },
            true,
            6.f,
            10.f,
            !ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows),
            [&]() { RenderSidebar(); },
            [&]() { RenderViewport(shouldRenderViewport); }
        );

        // Display material on drop
        Heart::ImGuiUtils::AssetDropTarget(
            Heart::Asset::Type::Material,
            [&](const Heart::HString8& path)
            { m_SelectedMaterial = Heart::AssetManager::RegisterAsset(Heart::Asset::Type::Material, path); }
        );

        ImGui::End();
        ImGui::PopStyleVar();
    }
    
    void MaterialEditor::Reset()
    {
        // Register an in-memory asset for the in-progress material
        m_EditingMaterialAsset = Heart::AssetManager::RegisterInMemoryAsset(Heart::Asset::Type::Material);
    }

    void MaterialEditor::SetSelectedMaterial(Heart::UUID material)
    {
        m_SelectedMaterial = material;
    }

    void MaterialEditor::SetSelectedMaterial(const Heart::Material& material)
    {
        m_LastMaterial = 0;
        m_SelectedMaterial = m_EditingMaterialAsset;
        auto editingMaterialAsset = Heart::AssetManager::RetrieveAsset<Heart::MaterialAsset>(m_EditingMaterialAsset);
        editingMaterialAsset->GetMaterial() = material;
    }

    void MaterialEditor::RenderSidebar()
    {
        if (!m_SelectedMaterial)
        {
            ImGui::TextColored({ 1.f, 1.f, 0.f, 1.f }, "No Material Selected");
            return;
        }

        auto materialAsset = Heart::AssetManager::RetrieveAsset<Heart::MaterialAsset>(m_SelectedMaterial);
        if (materialAsset && materialAsset->Load()->IsValid())
        {
            bool isResource = Heart::AssetManager::IsAssetAResource(m_SelectedMaterial);
            bool isTemporary = m_SelectedMaterial == m_EditingMaterialAsset;
            bool canEdit = !isResource && !isTemporary;

            // Do not allow modification of engine resources, but still allow for them to be displayed

            auto editingMaterialAsset = Heart::AssetManager::RetrieveAsset<Heart::MaterialAsset>(m_EditingMaterialAsset);
            auto& editingMaterial = editingMaterialAsset->GetMaterial();
            auto& materialData = editingMaterial.GetMaterialData();

            // Material path
            if (!materialAsset->GetPath().IsEmpty())
                ImGui::Text("%s", materialAsset->GetPath().Data());

            // Status
            if (isTemporary)
                ImGui::TextColored({ 1.f, 1.f, 0.f, 1.f }, "Cannot modify a temporary resource");
            else if (isResource)
                ImGui::TextColored({ 1.f, 1.f, 0.f, 1.f }, "Cannot modify an engine resource");

            // Save buttons
            bool disabled = !m_Dirty || !canEdit;
            if (disabled)
                ImGui::BeginDisabled();
            if (ImGui::Button("Save Changes"))
            {
                m_Dirty = false;
                materialAsset->Save(editingMaterial);
            }
            ImGui::SameLine();
            if (ImGui::Button("Revert Changes"))
            {
                m_Dirty = false;
                editingMaterial = materialAsset->GetMaterial();
            }
            ImGui::Dummy({ 0.f, 5.f });
            if (disabled)
                ImGui::EndDisabled();

            ImGui::Text("Material Properties");
            ImGui::Separator();

            if (!canEdit)
                ImGui::BeginDisabled();

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

            ImGui::Text("Transparency Mode:");
            ImGui::SameLine();
            bool popupOpened = ImGui::Button(Heart::TransparencyModeStrings[(u8)editingMaterial.GetTransparencyMode()]);
            if (popupOpened)
                ImGui::OpenPopup("tpModeSelect");
        
            if (ImGui::BeginPopup("tpModeSelect"))
            {
                for (u32 i = 0; i < Heart::TransparencyModeStrings.size(); i++)
                {
                    if (ImGui::MenuItem(Heart::TransparencyModeStrings[i]))
                    {
                        editingMaterial.SetTransparencyMode((Heart::TransparencyMode)i);
                        m_Dirty = true;
                    }
                }
                ImGui::EndPopup();
            }

            float previewSize = 128.f;
            ImGui::Dummy({ 0.f, 5.f });
            ImGui::Text("Material Textures");
            ImGui::Separator();

            if (!canEdit)
                ImGui::EndDisabled();

            // ---------------------------------
            // Albedo select
            // ---------------------------------
            ImGui::Text("Albedo:");
            ImGui::SameLine();
            ImGui::PushID("albedo");
            m_AssetPicker.OnImGuiRender(
                editingMaterial.GetAlbedoTexture(),
                Heart::Asset::Type::Texture,
                "None",
                [&]()
                {
                    if (!canEdit) return;

                    if (ImGui::MenuItem("Clear"))
                    {
                        editingMaterial.SetAlbedoTexture(0);
                        m_Dirty = true;
                    }
                },
                [&](Heart::UUID selected)
                {
                    if (!canEdit) return;

                    editingMaterial.SetAlbedoTexture(selected);
                    m_Dirty = true;
                }
            );
            ImGui::PopID();
            auto albedoAsset = Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(editingMaterial.GetAlbedoTexture());
            if (albedoAsset && albedoAsset->Load(false)->IsValid())
                ImGui::Image(albedoAsset->GetTexture()->GetImGuiHandle(0), { previewSize, previewSize });

            ImGui::Separator();

            // ---------------------------------
            // MetallicRoughness select
            // ---------------------------------
            ImGui::Text("Metallic Roughness:");
            ImGui::SameLine();
            ImGui::PushID("mr");
            m_AssetPicker.OnImGuiRender(
                editingMaterial.GetMetallicRoughnessTexture(),
                Heart::Asset::Type::Texture,
                "None",
                [&]()
                {
                    if (!canEdit) return;

                    if (ImGui::MenuItem("Clear"))
                    {
                        editingMaterial.SetMetallicRoughnessTexture(0);
                        m_Dirty = true;
                    }
                },
                [&](Heart::UUID selected)
                {
                    if (!canEdit) return;

                    editingMaterial.SetMetallicRoughnessTexture(selected);
                    m_Dirty = true;
                }
            );
            ImGui::PopID();
            auto mrAsset = Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(editingMaterial.GetMetallicRoughnessTexture());
            if (mrAsset && mrAsset->Load(false)->IsValid())
                ImGui::Image(mrAsset->GetTexture()->GetImGuiHandle(0), { previewSize, previewSize });

            ImGui::Separator();

            // ---------------------------------
            // Normal select
            // ---------------------------------
            ImGui::Text("Normal:");
            ImGui::SameLine();
            ImGui::PushID("normal");
            m_AssetPicker.OnImGuiRender(
                editingMaterial.GetNormalTexture(),
                Heart::Asset::Type::Texture,
                "None",
                [&]()
                {
                    if (!canEdit) return;

                    if (ImGui::MenuItem("Clear"))
                    {
                        editingMaterial.SetNormalTexture(0);
                        m_Dirty = true;
                    }
                },
                [&](Heart::UUID selected)
                {
                    if (!canEdit) return;

                    editingMaterial.SetNormalTexture(selected);
                    m_Dirty = true;
                }
            );
            ImGui::PopID();
            auto normalAsset = Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(editingMaterial.GetNormalTexture());
            if (normalAsset && normalAsset->Load(false)->IsValid())
                ImGui::Image(normalAsset->GetTexture()->GetImGuiHandle(0), { previewSize, previewSize });

            ImGui::Separator();

            // ---------------------------------
            // Emissive select
            // ---------------------------------
            ImGui::Text("Emissive:");
            ImGui::SameLine();
            ImGui::PushID("emiss");
            m_AssetPicker.OnImGuiRender(
                editingMaterial.GetEmissiveTexture(),
                Heart::Asset::Type::Texture,
                "None",
                [&]()
                {
                    if (!canEdit) return;

                    if (ImGui::MenuItem("Clear"))
                    {
                        editingMaterial.SetEmissiveTexture(0);
                        m_Dirty = true;
                    }
                },
                [&](Heart::UUID selected)
                {
                    if (!canEdit) return;

                    editingMaterial.SetEmissiveTexture(selected);
                    m_Dirty = true;
                }
            );
            ImGui::PopID();
            auto emAsset = Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(editingMaterial.GetEmissiveTexture());
            if (emAsset && emAsset->Load(false)->IsValid())
                ImGui::Image(emAsset->GetTexture()->GetImGuiHandle(0), { previewSize, previewSize });

            ImGui::Separator();

            // ---------------------------------
            // Occlusion select
            // ---------------------------------
            ImGui::Text("Occlusion:");
            ImGui::SameLine();
            ImGui::PushID("occ");
            m_AssetPicker.OnImGuiRender(
                editingMaterial.GetOcclusionTexture(),
                Heart::Asset::Type::Texture,
                "None",
                [&]()
                {
                    if (!canEdit) return;

                    if (ImGui::MenuItem("Clear"))
                    {
                        editingMaterial.SetOcclusionTexture(0);
                        m_Dirty = true;
                    }
                },
                [&](Heart::UUID selected)
                {
                    if (!canEdit) return;

                    editingMaterial.SetOcclusionTexture(selected);
                    m_Dirty = true;
                }
            );
            ImGui::PopID();
            auto ocAsset = Heart::AssetManager::RetrieveAsset<Heart::TextureAsset>(editingMaterial.GetOcclusionTexture());
            if (ocAsset && ocAsset->Load(false)->IsValid())
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
            m_SceneRenderer->GetOutputTexture()->GetImGuiHandle(),
            { viewportSize.x, viewportSize.y }
        );

        // If we are dragging in the viewport, update the swivel camera rotation
        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseDragging(0))
            {
                glm::vec2 delta = Heart::Input::GetMouseDelta();
                m_SwivelRotation.y += delta.x;
                m_SwivelRotation.x += -delta.y;
            }

            // Camera will orbit around { 0, 0, 0 }
            f32 scrollY = (f32)Heart::Input::GetAxisDelta(Heart::AxisCode::ScrollY);
            m_Radius = std::clamp(m_Radius + static_cast<float>(-scrollY), 0.f, 100.f);
            m_SceneCamera.UpdateViewMatrix(glm::vec3(0.f), m_Radius, m_SwivelRotation);
            m_SceneCameraPosition = -m_SceneCamera.GetForwardVector() * m_Radius;
        }
    }
}
}
