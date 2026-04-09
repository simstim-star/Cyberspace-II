#include "../core/pch.h"


#include "ui.h"
#include "../assets/asset_loader.h"
#include "../core/scene.h"
#include "../shaders/sendai/shader_defs.h"
#include "../win32/file_dialog.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

Sendai::UI::UI(HWND hWnd, ID3D12Device *Device)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    io.Fonts->AddFontDefault();
    io.Fonts->Build();

    D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeap = {};
    DescriptorHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    DescriptorHeap.NumDescriptors = 1;
    DescriptorHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    Device->CreateDescriptorHeap(&DescriptorHeap, IID_PPV_ARGS(SrvHeap.GetAddressOf()));

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX12_Init(Device, FRAME_COUNT, DXGI_FORMAT_R8G8B8A8_UNORM, SrvHeap.Get(),
                        SrvHeap->GetCPUDescriptorHandleForHeapStart(), SrvHeap->GetGPUDescriptorHandleForHeapStart());

    ImGui_ImplDX12_CreateDeviceObjects();
}

VOID Sendai::UI::PrepareData(Renderer &Renderer, Scene &Scene)
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    UI::_DrawTopBar(Renderer, Scene);
    UI::_DrawSceneInspector(Scene);

    // Prepare the data for rendering, doesn't touch GPU or graphics API
    // ImGui_ImplXXXX_RenderDrawData() provided by the renderer backend that will actually draw
    ImGui::Render();
}

VOID Sendai::UI::Draw(ID3D12GraphicsCommandList *CommandList)
{
    ID3D12DescriptorHeap *ImGuiHeaps[] = {SrvHeap.Get()};
    CommandList->SetDescriptorHeaps(_countof(ImGuiHeaps), ImGuiHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), CommandList);
}

void Sendai::UI::_DrawSceneInspector(Sendai::Scene &Scene)
{
    ImGui::Begin("Sendai Scene Inspector");

    if (ImGui::CollapsingHeader("Scene Hierarchy", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Models");

        if (Scene.Models.empty())
        {
            ImGui::TextDisabled("No models loaded.");
        }
        else
        {
            ImGui::BeginChild("ModelListChild", ImVec2(150, 200), true);
            for (int i = 0; i < (int)Scene.Models.size(); ++i)
            {
                std::string Label = "Model " + std::to_string(i);
                if (ImGui::Selectable(Label.c_str(), SelectedModelIndex == i))
                {
                    SelectedModelIndex = i;
                    SelectedLightIndex = -1;
                }
            }
            ImGui::EndChild();
        }
        ImGui::SameLine();

        if (SelectedModelIndex >= 0 && SelectedModelIndex < (int)Scene.Models.size())
        {
            auto &Model = Scene.Models[SelectedModelIndex];

            ImGui::BeginChild("ModelPropertiesChild", ImVec2(0, 200), true);
            ImGui::Text("Model %d Properties", SelectedModelIndex);
            ImGui::Separator();

            ImGui::DragFloat3("Position", &Model.Position.x, 0.1f);
            ImGui::DragFloat3("Rotation", &Model.Rotation.x, 0.01f);
            ImGui::DragFloat3("Scale", &Model.Scale.x, 0.05f);

            ImGui::Checkbox("Visible", (bool *)&Model.bVisible);

            ImGui::EndChild();
        }
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Active Lights (PBR Mask: 0x%02X)", Scene.ActiveLightMask);
        ImGui::BeginChild("LightSelectionChild", ImVec2(0, 150), true);

        for (int i = 0; i < PBR_MAX_LIGHT_NUMBER; ++i)
        {
            ImGui::PushID(i);

            const auto &Light = Scene.Data.Lights[i];
            ImVec4 LightColor = ImVec4(Light.LightColor.x, Light.LightColor.y, Light.LightColor.z, 1.0f);
            std::string Label = "Light " + std::to_string(i);

            if (ImGui::Selectable(Label.c_str(), SelectedLightIndex == i, 0, ImVec2(0, 30)))
            {
                SelectedLightIndex = i;
                SelectedModelIndex = -1;
            }
            ImGui::SameLine();
            ImGui::ColorButton("##Color", LightColor, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip,
                               ImVec2(20, 20));

            ImGui::PopID();
        }
        ImGui::EndChild();

        if (SelectedLightIndex >= 0 && SelectedLightIndex < PBR_MAX_LIGHT_NUMBER)
        {
            auto &Light = Scene.Data.Lights[SelectedLightIndex];

            ImGui::Separator();
            ImGui::Text("Light %d Configuration", SelectedLightIndex);

            bool bActive = (Scene.ActiveLightMask & (1 << SelectedLightIndex)) != 0;
            if (ImGui::Checkbox("Is Active", &bActive))
            {
                if (bActive)
                    Scene.ActiveLightMask |= (1 << SelectedLightIndex);
                else
                    Scene.ActiveLightMask &= ~(1 << SelectedLightIndex);
            }

            ImGui::DragFloat3("Position", &Light.LightPosition.x, 0.1f);
            ImGui::ColorEdit3("Color/Tint", &Light.LightColor.x);
        }
    }

    ImGui::End();
}

void Sendai::UI::_DrawTopBar(Sendai::Renderer &Renderer, Sendai::Scene &Scene)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open Model...", "Ctrl+O"))
            {
                PWSTR SelectedPath = Win32SelectGLTFPath();

                if (SelectedPath != nullptr)
                {
                    std::wstring wPath(SelectedPath);
                    Scene.Models.push_back(Sendai::Model{});
                    if (!Sendai::LoadModel(&Renderer, wPath, Scene))
                    {
                        Scene.Models.pop_back();
                    }
                    CoTaskMemFree(SelectedPath);
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit", "Alt+F4"))
            {
                PostQuitMessage(0);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("Grid", nullptr, &Renderer.bDrawGrid);
            ImGui::EndMenu();
        }

        FLOAT FrameTime = 1000.0f / ImGui::GetIO().Framerate;
        ImGui::SameLine(ImGui::GetWindowWidth() - 150);
        ImGui::Text("%.2f ms/f (%.1f FPS)", FrameTime, ImGui::GetIO().Framerate);

        ImGui::EndMainMenuBar();
    }
}