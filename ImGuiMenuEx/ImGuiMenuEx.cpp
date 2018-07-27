// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

//#include "imgui/imgui.h"
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_dx9.h"
#include "imgui/examples/imgui_impl_win32.h"



using Present_t = HRESULT(APIENTRY*) (IDirect3DDevice9*, const RECT *, const RECT *, HWND, const RGNDATA *);
HRESULT APIENTRY Present_Desvio(IDirect3DDevice9*, const RECT *, const RECT *, HWND, const RGNDATA *);
Present_t Present_Original = nullptr;

using Reset_t = HRESULT(APIENTRY *)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
HRESULT APIENTRY Reset_Desvio(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
Reset_t Reset_Original = nullptr;



WNDPROC game_wndproc = nullptr;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

HWND GameHWND = nullptr;
bool doIni = true;


bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
HRESULT APIENTRY Present_Desvio(IDirect3DDevice9* pDevice, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion)
{
	if (doIni)
	{
		doIni = false;
		IMGUI_CHECKVERSION();
		game_wndproc = (WNDPROC)(SetWindowLongPtr(GameHWND, GWLP_WNDPROC, LONG_PTR(WndProc)));
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
		ImGui_ImplWin32_Init(GameHWND);
		ImGui_ImplDX9_Init(pDevice);

		// Setup style
		ImGui::StyleColorsDark();
	}
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}

	// Rendering
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	HRESULT ret = Present_Original(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);


	return ret;
}

HRESULT APIENTRY Reset_Desvio(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT ResetReturn = Reset_Original(pDevice, pPresentationParameters);
	if (SUCCEEDED(ResetReturn))
	{
		ImGui_ImplDX9_CreateDeviceObjects();
		ImGui_ImplDX9_Shutdown();
		ImGui::DestroyContext();
	}
	return ResetReturn;
}



//==========================================================================================================================


bool CriarDevice(UINT_PTR * dVTable) {
	LPDIRECT3D9 mD3D = nullptr;
	mD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (mD3D == nullptr)
		return false;

	D3DPRESENT_PARAMETERS pPresentParams;
	ZeroMemory(&pPresentParams, sizeof(pPresentParams));
	pPresentParams.Windowed = true;
	pPresentParams.BackBufferFormat = D3DFMT_UNKNOWN;
	pPresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;

	LPDIRECT3DDEVICE9 pDevice = nullptr;
	if (FAILED(mD3D->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		GetDesktopWindow(),
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&pPresentParams,
		&pDevice)))
		return false;
	auto *vTable = (UINT_PTR*)pDevice;
	vTable = (UINT_PTR*)vTable[0];
	dVTable[0] = vTable[16];  //Reset
	dVTable[1] = vTable[17];  //Present
	dVTable[2] = vTable[32];  //GetRenderTargetData
	dVTable[3] = vTable[36];  //CreateOffscreenPlainSurface
	dVTable[4] = vTable[65];  //SetTexture
	dVTable[5] = vTable[100]; //SetStreamSource
	dVTable[6] = vTable[42]; //SetStreamSource
	pDevice->Release();
	mD3D->Release();
	return true;
}
UINT_PTR vTable[7] = { 0 };


bool setHooks()
{
	if (MH_Initialize() != MH_OK) { return false; }
	if (MH_CreateHook((DWORD_PTR*)vTable[1], &Present_Desvio, reinterpret_cast<void**>(&Present_Original)) != MH_OK) { return false; }
	if (MH_EnableHook((DWORD_PTR*)vTable[1]) != MH_OK) { return false; }

	if (MH_CreateHook((DWORD_PTR*)vTable[0], &Reset_Desvio, reinterpret_cast<void**>(&Reset_Original)) != MH_OK) { return false; }
	if (MH_EnableHook((DWORD_PTR*)vTable[0]) != MH_OK) { return false; }
	return true;
}
void doThread()
{
	if (CriarDevice(vTable))
	{
		printf("Reset: 0x%X\n", (DWORD)vTable[0]);
		printf("Present: 0x%X\n", (DWORD)vTable[1]);
		GameHWND = FindWindowA("TestD3D9", nullptr);//janela do jogo
		setHooks();
	}

}
void OpenConsole(std::string Title)
{
	AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	SetConsoleTitle(Title.c_str());
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		OpenConsole("");
		CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)doThread, nullptr, 0, nullptr);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

