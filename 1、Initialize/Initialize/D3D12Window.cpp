#include "D3D12Window.h"

const TCHAR* D3D12Window::ClassName = TEXT("D3D12Window");

LRESULT CALLBACK D3D12Window::GlobalWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_NCCREATE) {
		CREATESTRUCT* ps = (CREATESTRUCT*)lparam;
		D3D12Window* wnd = (D3D12Window*)ps->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)wnd);
	}
	D3D12Window* wnd = (D3D12Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	return wnd == nullptr ? DefWindowProc(hwnd, msg, wparam, lparam) : wnd->WndProc(hwnd, msg, wparam, lparam);
}

D3D12Window::D3D12Window(const TCHAR* title)
{
	hwnd = CreateWindowEx(0, ClassName, title, WS_SYSMENU | WS_MINIMIZEBOX, 200, 200, 1280, 720, nullptr, nullptr, nullptr, this);
	resources.Initialize(hwnd);
}

int D3D12Window::Run()
{
	ShowWindow(hwnd, SW_SHOW);
	MSG msg;
	while (GetMessage(&msg, hwnd, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

LRESULT D3D12Window::WndProc(HWND hwnd, UINT msg, WPARAM wparm, LPARAM lparm)
{
	switch (msg)
	{
	case WM_PAINT:
		Rendering();
		return 1;
	case WM_ERASEBKGND:
		return 1;
	}
	return DefWindowProc(hwnd, msg, wparm, lparm);
}

void D3D12Window::Rendering()
{
	resources.pCommandAllocator->Reset();
	resources.pCommandList->Reset(resources.pCommandAllocator, nullptr);
	static const float color[] = { 0.0f, 0.5f, 0.5f, 1 };
	auto&& rtv = resources.CurrentBackBufferView();
	auto&& dsv = resources.DepthStencilView();
	resources.pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resources.buffers[resources.mCurrBackBuffer], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	resources.pCommandList->RSSetViewports(1, &resources.mScreenViewport);
	resources.pCommandList->RSSetScissorRects(1, &resources.mScissorRect);
	resources.pCommandList->OMSetRenderTargets(1, &rtv, true, &dsv);
	resources.pCommandList->ClearRenderTargetView(rtv, color, 0, nullptr);
	resources.pCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1, 0, 0, nullptr);
	resources.pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resources.buffers[resources.mCurrBackBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	resources.pCommandList->Close();
	resources.pCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&resources.pCommandList);
	resources.pSwapChain->Present(1, 0);
	++resources.mCurrBackBuffer %= resources.buffers.size();
	resources.FlushCommandQueue();
}
