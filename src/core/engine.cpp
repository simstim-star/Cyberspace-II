#include "pch.h"

#include "engine.h"
#include "../renderer/light.h"
#include "../renderer/renderer.h"
#include "../renderer/shader.h"
#include "../renderer/texture.h"
#include "../win32/file_dialog.h"
#include "../win32/win_path.h"
#include "../assets/asset_loader.h"
#include "../ui/ui.h"

#include <imgui_impl_win32.h>

using namespace Microsoft::WRL;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK _WindowProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

Sendai::App::App(std::wstring Title)
    : Window{Title, 900, 600}, bRunning{TRUE}, Camera{{0, 3, -20}},
      FrameCounter{0}
{
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_CLASSDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = _WindowProc;
    wc.hInstance = Window.hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIconSm = LoadIcon(nullptr, IDI_WINLOGO);
    wc.lpszClassName = L"SendaiClass";
    RegisterClassEx(&wc);
    RECT rect = {0, 0, static_cast<LONG>(Window.Width), static_cast<LONG>(Window.Height)};
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    Window.hWnd =
        CreateWindow(wc.lpszClassName, Title.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                     rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, Window.hInstance, this);

    Scene.Models.reserve(10000);
    RendererCore = std::make_unique<Renderer>(Window.hWnd, Window.Width, Window.Height);
    UI = std::make_unique<Sendai::UI>(Window.hWnd, RendererCore->Device.Get());
}

VOID Sendai::App::Update()
{
    if (FrameCounter == 50)
    {
        FrameCounter = 0;
    }
    FrameCounter++;

    Timer.Tick();
    Camera.Update(Timer.ElapsedSeconds());
}

INT Sendai::Run()
{
    Sendai::App App{
        L"Sendai",
    };

    LightsInit(App.Scene, App.Camera);

    ShowWindow(App.Window.hWnd, SW_MAXIMIZE);

    MSG Message = {0};
    while (App.bRunning)
    {
        while (PeekMessage(&Message, nullptr, 0, 0, PM_REMOVE))
        {
            if (Message.message == WM_QUIT)
            {
                App.bRunning = FALSE;
            }
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }

        if (App.bRunning)
        {
            App.Update();
            App.UI->PrepareData(*App.RendererCore.get(), App.Scene);
            App.RendererCore->BeginDraw(App.Scene, App.Camera);
            App.UI->Draw(App.RendererCore->CommandList.Get());
            App.RendererCore->EndDraw();
        }
    }

    return (INT)(Message.wParam);
}

VOID Sendai::AfterRun()
{
#if defined(_DEBUG)
    ComPtr<IDXGIDebug1> DebugDev;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(DebugDev.GetAddressOf()))))
    {
        DebugDev->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }
#endif
}

VOID Sendai::Callback::DoNothing(Sendai::App *const Engine)
{
}

VOID Sendai::Callback::FileOpen(Sendai::App *const Engine)
{
    std::wstring FilePath = Win32SelectGLTFPath();

    Sendai::Scene &Scene = Engine->Scene;
    Sendai::LoadModel(Engine->RendererCore.get(), FilePath, Scene);
    UINT ModelIdx = Scene.Models.size() - 1;
}

VOID Sendai::Callback::WireframeMode(Sendai::App *const Engine)
{
    if (Engine->RendererCore->State != ERS_WIREFRAME)
    {
        Engine->RendererCore->State = ERS_WIREFRAME;
    }
    else
    {
        Engine->RendererCore->State = ERS_GLTF;
    }
}

VOID Sendai::Callback::GridMode(Sendai::App *const Engine)
{
    Engine->RendererCore->bDrawGrid = !Engine->RendererCore->bDrawGrid;
}

LRESULT CALLBACK _WindowProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    Sendai::App *Engine = (Sendai::App *)(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (ImGui_ImplWin32_WndProcHandler(hWnd, Message, wParam, lParam))
        return TRUE;

    switch (Message)
    {
    case WM_CREATE: {
        LPCREATESTRUCT pCreateStruct = (LPCREATESTRUCT)(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)(pCreateStruct->lpCreateParams));
        return 0;
    }
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
        {
            return 0;
        }
        if (Engine && Engine->RendererCore && Engine->RendererCore->SwapChain)
        {
            int Width = LOWORD(lParam);
            int Height = HIWORD(lParam);
            Engine->RendererCore->SwapchainResize(Width, Height);
        }
        return 0;

    case WM_PAINT:
        ValidateRect(hWnd, NULL);
        return 0;

    case WM_KEYDOWN:
        if (Engine)
        {
            Engine->Camera.OnKeyDown(wParam);
        }
        return 0;

    case WM_KEYUP:
        if (Engine)
        {
            Engine->Camera.OnKeyUp(wParam);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, Message, wParam, lParam);
}
