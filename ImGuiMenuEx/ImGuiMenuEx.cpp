#include "stdafx.h"
#include "dx/dx9.h"
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_dx9.h"
#include "imgui/examples/imgui_impl_win32.h"


WNDPROC p_window_proc = nullptr;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK wnd_proc(const HWND hwnd, const UINT u_msg, const WPARAM w_param, const LPARAM l_param)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, u_msg, w_param, l_param) && GetAsyncKeyState(VK_INSERT) & 1)
	{
		return 1l;
	}
	return CallWindowProc(p_window_proc, hwnd, u_msg, w_param, l_param);
}

bool do_ini = true;


bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
void __stdcall main_render(IDirect3DDevice9* p_device)
{
	//printf("zz\n");
	if (do_ini)
	{
		const auto window = GetForegroundWindow();
		DWORD pid;
		if (!window || !GetWindowThreadProcessId(window, &pid) || pid != GetCurrentProcessId())
			return;	
		do_ini = false;
		p_window_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(window, GWLP_WNDPROC, LONG_PTR(wnd_proc)));
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(window);
		ImGui_ImplDX9_Init(p_device);
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
}

auto gaks	= reinterpret_cast<void*>(&GetAsyncKeyState);
auto gks	= reinterpret_cast<void*>(&GetKeyState);
void xigncode3_un_hook()
{
	DWORD old;
	const size_t alloc_size = 100;
	const auto alloc_gaks	= malloc(alloc_size);
	const auto alloc_gks	= malloc(alloc_size);
	if (!alloc_gaks || !alloc_gks)
		return;

	memcpy(alloc_gaks,	gaks,	alloc_size);
	memcpy(alloc_gks,	gks,	alloc_size);
	VirtualProtect(gaks,	alloc_size, PAGE_EXECUTE_READWRITE, &old);
	VirtualProtect(gks,		alloc_size, PAGE_EXECUTE_READWRITE, &old);
	while (true)
	{
		if (memcmp(gaks, alloc_gaks, alloc_size) != 0 && memcmp(gks, alloc_gks, alloc_size) != 0)
		{
			memcpy(gaks,	alloc_gaks, alloc_size);
			memcpy(gks,		alloc_gks,	alloc_size);
			break;
		}
		Sleep(100);
	}
	free(alloc_gaks);
	free(alloc_gks);
}

void do_thread()
{
	printf("ini\n");
	dx9::set_frame_render(reinterpret_cast<void*>( main_render ));
	xigncode3_un_hook();
}

void open_console(const std::string title)
{
	AllocConsole();
	FILE* street;
	freopen_s(&street, "CONIN$", "r", stdin);
	freopen_s(&street, "CONOUT$", "w", stdout);
	freopen_s(&street, "CONOUT$", "w", stderr);
	SetConsoleTitle(title.c_str());
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
