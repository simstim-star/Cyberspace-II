#include "pch.h"

#include "../assets/gltf.h"
#include "../renderer/light.h"
#include "../renderer/renderer.h"
#include "../renderer/shader.h"
#include "../renderer/texture.h"
#include "../win32/file_dialog.h"
#include "../win32/win_path.h"
#include "engine.h"

/****************************************************
	Forward declaration of private functions
*****************************************************/

LRESULT CALLBACK WindowProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

/****************************************************
	Public functions
*****************************************************/

Sendai::App::App(std::wstring Title)
	: Window{Title, 900, 600}, bRunning{TRUE}, Camera{{0, 3, -20}},
	  Scene{
		.SceneArena = M_ArenaInit(MEGABYTES(512)),
	  },
	  FrameCounter{0}
{
	WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_CLASSDC | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = Window.hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
	wc.lpszClassName = L"SendaiClass";
	RegisterClassEx(&wc);
	RECT rect = {0, 0, static_cast<LONG>(Window.Width), static_cast<LONG>(Window.Height)};
	AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
	Window.hWnd = CreateWindow(wc.lpszClassName, Title.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
							   rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, Window.hInstance, this);

	Scene.Models.reserve(10000);
	RendererCore = std::make_unique<Renderer>(Window.hWnd, Window.Width, Window.Height);
}

VOID
Sendai::App::Update()
{
	if (FrameCounter == 50) {
		FrameCounter = 0;
	}
	FrameCounter++;

	Timer.Tick();
	Camera.Update(Timer.ElapsedSeconds());
}

INT
Sendai::Run()
{
	Sendai::App App{
	  L"Sendai",
	};

	R_LightsInit(App.Scene, App.Camera);

	ShowWindow(App.Window.hWnd, SW_MAXIMIZE);

	MSG msg = {0};
	while (App.bRunning) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				App.bRunning = FALSE;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (App.bRunning) {
			App.Update();
			App.RendererCore->Draw(App.Scene, App.Camera);
		}
	}

	M_ArenaRelease(&App.Scene.SceneArena);

	return (INT)(msg.wParam);
}

void
Sendai::AfterRun()
{
#if defined(_DEBUG)
	ComPtr<IDXGIDebug1> DebugDev;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(DebugDev.GetAddressOf())))) {
		DebugDev->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
	}
#endif
}

void
Sendai::Callback::DoNothing(Sendai::App *const Engine)
{
}

void
Sendai::Callback::FileOpen(Sendai::App *const Engine)
{
	std::wstring FilePath = Win32SelectGLTFPath();

	Sendai::Scene &Scene = Engine->Scene;
	if (Scene.UploadArena.Base) {
		M_ArenaRelease(&Scene.UploadArena);
	}

	Scene.UploadArena = M_ArenaInit(GIGABYTES(1));
	SendaiGLTF_LoadModel(Engine->RendererCore.get(), FilePath, Scene);
	UINT ModelIdx = Scene.Models.size() - 1;
	M_ArenaRelease(&Scene.UploadArena);
}

void
Sendai::Callback::WireframeMode(Sendai::App *const Engine)
{
	if (Engine->RendererCore->State != ERS_WIREFRAME) {
		Engine->RendererCore->State = ERS_WIREFRAME;
	} else {
		Engine->RendererCore->State = ERS_GLTF;
	}
}

void
Sendai::Callback::GridMode(Sendai::App *const Engine)
{
	Engine->RendererCore->bDrawGrid = !Engine->RendererCore->bDrawGrid;
}

/****************************************************
	Implementation of private functions
*****************************************************/

LRESULT CALLBACK
WindowProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	Sendai::App *Engine = (Sendai::App *)(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (Message) {
	case WM_CREATE: {
		LPCREATESTRUCT pCreateStruct = (LPCREATESTRUCT)(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)(pCreateStruct->lpCreateParams));
		return 0;
	}
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED) {
			return 0;
		}
		if (Engine && Engine->RendererCore && Engine->RendererCore->SwapChain) {
			int Width = LOWORD(lParam);
			int Height = HIWORD(lParam);
			Engine->RendererCore->SwapchainResize(Width, Height);
		}
		return 0;

	case WM_PAINT:
		ValidateRect(hWnd, NULL);
		return 0;

	case WM_KEYDOWN:
		if (Engine) {
			Engine->Camera.OnKeyDown(wParam);
		}
		return 0;

	case WM_KEYUP:
		if (Engine) {
			Engine->Camera.OnKeyUp(wParam);
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, Message, wParam, lParam);
}
