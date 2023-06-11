#include "D3D12Window.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	WNDCLASS cls{};
	cls.hInstance = hInstance;
	cls.lpszClassName = D3D12Window::ClassName;
	cls.lpfnWndProc = D3D12Window::GlobalWndProc;
	RegisterClass(&cls);

	D3D12Window app(TEXT("Initialize"));
	return app.Run();
}