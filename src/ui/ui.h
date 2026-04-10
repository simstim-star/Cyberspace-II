#pragma once
#include "../core/scene.h"

namespace Sendai
{
class UI
{
  public:
    UI(HWND hWnd, ID3D12Device *Device);
    VOID PrepareData(Renderer &Renderer, Scene &Scene);
    VOID Draw(ID3D12GraphicsCommandList *CommandList);
  
private:
    VOID _DrawSceneInspector(Scene &Scene);
    VOID _DrawTopBar(Renderer &Renderer, Scene &Scene);

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> SrvHeap;
    INT SelectedModelIndex{-1};
    INT SelectedLightIndex{-1};
};
} // namespace Sendai
