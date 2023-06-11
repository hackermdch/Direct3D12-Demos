#pragma once
#include <Windows.h>
#include "DeviceResources.h"

class D3D12Window
{
public:
	static const TCHAR* ClassName;
	static LRESULT CALLBACK GlobalWndProc(HWND hwnd, UINT msg, WPARAM wparm, LPARAM lparm);
	D3D12Window(const TCHAR* title);
	int Run();
private:
	HWND hwnd;
	DeviceResources resources;
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wparm, LPARAM lparm);
	void Rendering();
};

