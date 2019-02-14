#include "stdafx.h"
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_dx9.h"
#include "imgui/examples/imgui_impl_win32.h"
#include "Detour4.0/include/detours.h"
#if _WIN64
#pragma comment(lib, "Detour4.0//libs//x64_//detours.lib")
#else
#pragma comment(lib, "Detour4.0//libs//x86_//detours.lib")
#endif


using present_t = HRESULT(APIENTRY*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
HRESULT APIENTRY Present_Desvio(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
present_t present_original = nullptr;

using reset_t = HRESULT(APIENTRY *)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
HRESULT APIENTRY Reset_Desvio(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
reset_t reset_original = nullptr;


WNDPROC game_wndproc = nullptr;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK wnd_proc(const HWND hwnd, const UINT u_msg, const WPARAM w_param, const LPARAM l_param)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, u_msg, w_param, l_param) && GetAsyncKeyState(VK_INSERT) & 1)
	{
		return 1l;
	}
	return CallWindowProc(game_wndproc, hwnd, u_msg, w_param, l_param);
}

HWND game_hwnd = nullptr;
bool do_ini = true;


bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

HRESULT APIENTRY Present_Desvio(IDirect3DDevice9* p_device, const RECT* p_source_rect, const RECT* p_dest_rect,
                                const HWND h_dest_window_override, const RGNDATA* p_dirty_region)
{
	if (do_ini)
	{
		do_ini = false;
		IMGUI_CHECKVERSION();
		game_wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(game_hwnd, GWLP_WNDPROC, LONG_PTR(wnd_proc)));
		ImGui::CreateContext();
		auto& io = ImGui::GetIO();
		(void)io;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
		ImGui_ImplWin32_Init(game_hwnd);
		ImGui_ImplDX9_Init(p_device);

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
		static auto f = 0.0f;
		static auto counter = 0;

		ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text."); // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f    
		ImGui::ColorEdit3("clear color", reinterpret_cast<float*>(&clear_color)); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))
			// Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
		            ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);
		// Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}

	// Rendering
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	const auto ret = present_original(p_device, p_source_rect, p_dest_rect, h_dest_window_override, p_dirty_region);


	return ret;
}

HRESULT APIENTRY Reset_Desvio(IDirect3DDevice9* p_device, D3DPRESENT_PARAMETERS* p_presentation_parameters)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	const auto ResetReturn = reset_original(p_device, p_presentation_parameters);
	if (SUCCEEDED(ResetReturn))
	{
		ImGui_ImplDX9_CreateDeviceObjects();
		ImGui_ImplDX9_Shutdown();
		ImGui::DestroyContext();
	}
	return ResetReturn;
}


//==========================================================================================================================


bool criar_device(UINT_PTR* dVTable)
{
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
	auto* vTable = reinterpret_cast<uintptr_t*>(pDevice);
	vTable = reinterpret_cast<uintptr_t*>(vTable[0]);
	dVTable[0] = vTable[16]; //Reset
	dVTable[1] = vTable[17]; //Present
	dVTable[2] = vTable[32]; //GetRenderTargetData
	dVTable[3] = vTable[36]; //CreateOffscreenPlainSurface
	dVTable[4] = vTable[65]; //SetTexture
	dVTable[5] = vTable[100]; //SetStreamSource
	dVTable[6] = vTable[42]; //SetStreamSource
	pDevice->Release();
	mD3D->Release();
	return true;
}

uintptr_t vTable[7] = {0};


bool setHooks()
{
	present_original	= reinterpret_cast<present_t>	(vTable[1]);
	reset_original		= reinterpret_cast<reset_t>		(vTable[0]);
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)present_original, Present_Desvio);
	DetourAttach(&(PVOID&)reset_original,	Reset_Desvio);
	DetourTransactionCommit();
	return true;
}

void do_thread()
{
	do
	{
		game_hwnd = FindWindowA("CrossFire"/*"D3D9"*/, nullptr); //janela do jogo
		Sleep(100);
	} while (!game_hwnd);
	if (criar_device(vTable))
	{
		printf("Reset: 0x%p, Present: 0x%p\n", vTable[0], vTable[1]);
		setHooks();
	}


	//XignCode3 unhookAPI
	DWORD old;
	LPVOID gaks = &GetAsyncKeyState;
	LPVOID gks  = &GetKeyState;
	auto gaks_byte = *reinterpret_cast<DWORD64*>(gaks);
	auto gks_byte  = *reinterpret_cast<DWORD64*>(gks);
	VirtualProtect(gaks, 8, PAGE_EXECUTE_READWRITE, &old);
	VirtualProtect(gks,  8, PAGE_EXECUTE_READWRITE, &old);
	while (true)
	{
		if (gaks_byte != *reinterpret_cast<DWORD64*>(gaks))
			* reinterpret_cast<DWORD64*>(gaks) = gaks_byte; //unhook GetAsyncKeyState 

		if (gks_byte  != *reinterpret_cast<DWORD64*>(gks))
			* reinterpret_cast<DWORD64*>(gks) = gks_byte; //unhook GetAsyncKeyState 
		Sleep(500);
	}
}

void open_console(std::string Title)
{
	AllocConsole();
	FILE* ssttree;
	freopen_s(&ssttree, "CONIN$", "r", stdin);
	freopen_s(&ssttree, "CONOUT$", "w", stdout);
	freopen_s(&ssttree, "CONOUT$", "w", stderr);
	SetConsoleTitle(Title.c_str());
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		open_console("");
		CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(do_thread), nullptr, 0, nullptr);
	}
	return TRUE;
}
