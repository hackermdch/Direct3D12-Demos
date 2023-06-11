#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include "d3dx12.h"

class DeviceResources
{
	friend class D3D12Window;
public:
	HRESULT Initialize(HWND hwnd);
	void FlushCommandQueue();
	~DeviceResources();
private:
	IDXGIFactory1* factory = nullptr;
	ID3D12Device* device = nullptr;
	ID3D12Fence* fence = nullptr;
	ID3D12CommandQueue* pCommandQueue = nullptr;
	ID3D12GraphicsCommandList* pCommandList = nullptr;
	ID3D12CommandAllocator* pCommandAllocator = nullptr;
	IDXGISwapChain* pSwapChain = nullptr;
	ID3D12DescriptorHeap* pRTVHeap = nullptr;
	ID3D12DescriptorHeap* pDSVHeap = nullptr;
	std::vector<ID3D12Resource*> buffers;
	ID3D12Resource* pDepthStencilBuffer = nullptr;
	int mCurrentFence = 0;
	int mCurrBackBuffer = 0;
	D3D12_VIEWPORT mScreenViewport{};
	D3D12_RECT mScissorRect{};
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;
};

__forceinline D3D12_CPU_DESCRIPTOR_HANDLE DeviceResources::CurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(pRTVHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
}

__forceinline D3D12_CPU_DESCRIPTOR_HANDLE DeviceResources::DepthStencilView() const
{
	return pDSVHeap->GetCPUDescriptorHandleForHeapStart();
}