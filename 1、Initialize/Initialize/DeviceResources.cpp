#include "DeviceResources.h"

HRESULT DeviceResources::Initialize(HWND hwnd)
{
#if defined(DEBUG) || defined(_DEBUG) 
	{
		ID3D12Debug* debugController;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
		debugController->EnableDebugLayer();
		debugController->Release();
	}
#endif
	HRESULT res = ERROR_SUCCESS;
	CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device));
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	D3D12_COMMAND_QUEUE_DESC qd = {};
	qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	qd.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	device->CreateCommandQueue(&qd, IID_PPV_ARGS(&pCommandQueue));
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator));
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator, nullptr, IID_PPV_ARGS(&pCommandList));
	pCommandList->Close();
	RECT rect{};
	GetClientRect(hwnd, &rect);
	auto width = rect.right - rect.left;
	auto height = rect.bottom - rect.top;
	DXGI_MODE_DESC md{};
	md.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	md.Width = width;
	md.Height = height;
	md.RefreshRate = { 60,1 };
	md.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	md.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.OutputWindow = hwnd;
	sd.Windowed = true;
	sd.BufferDesc = md;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	factory->CreateSwapChain(pCommandQueue, &sd, &pSwapChain);

	D3D12_DESCRIPTOR_HEAP_DESC rd = {};
	rd.NumDescriptors = sd.BufferCount;
	rd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rd.NodeMask = 0;
	device->CreateDescriptorHeap(&rd, IID_PPV_ARGS(&pRTVHeap));
	D3D12_DESCRIPTOR_HEAP_DESC dd = {};
	dd.NumDescriptors = 1;
	dd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dd.NodeMask = 0;
	device->CreateDescriptorHeap(&dd, IID_PPV_ARGS(&pDSVHeap));
	buffers.resize(sd.BufferCount);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rh(pRTVHeap->GetCPUDescriptorHandleForHeapStart());
	UINT rs = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (UINT i = 0; i < sd.BufferCount; i++)
	{
		pSwapChain->GetBuffer(i, IID_PPV_ARGS(&buffers[i]));
		device->CreateRenderTargetView(buffers[i], nullptr, rh);
		rh.Offset(1, rs);
	}

	D3D12_RESOURCE_DESC dsd = {};
	dsd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	dsd.Alignment = 0;
	dsd.Width = md.Width;
	dsd.Height = md.Height;
	dsd.DepthOrArraySize = 1;
	dsd.MipLevels = 1;
	dsd.Format = DXGI_FORMAT_R24G8_TYPELESS;
	dsd.SampleDesc.Count = 1;
	dsd.SampleDesc.Quality = 0;
	dsd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	dsd.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	D3D12_CLEAR_VALUE oc;
	oc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	oc.DepthStencil.Depth = 1.0f;
	oc.DepthStencil.Stencil = 0;
	device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &dsd, D3D12_RESOURCE_STATE_COMMON, &oc, IID_PPV_ARGS(&pDepthStencilBuffer));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvd;
	dsvd.Flags = D3D12_DSV_FLAG_NONE;
	dsvd.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvd.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(pDepthStencilBuffer, &dsvd, DepthStencilView());
	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDepthStencilBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	pCommandList->Close();
	pCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&pCommandList);
	FlushCommandQueue();

	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = width;
	mScreenViewport.Height = height;
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;
	mScissorRect = { 0, 0, width, height };
	factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_PRINT_SCREEN);
	return res;
}

void DeviceResources::FlushCommandQueue()
{
	mCurrentFence++;
	pCommandQueue->Signal(fence, mCurrentFence);
	if (fence->GetCompletedValue() < mCurrentFence) {
		HANDLE eventHandle = _Notnull_ CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		fence->SetEventOnCompletion(mCurrentFence, eventHandle);
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

DeviceResources::~DeviceResources()
{
	for (auto item : buffers) item->Release();
	pRTVHeap->Release();
	pDSVHeap->Release();
	pSwapChain->Release();
	pCommandList->Release();
	pCommandAllocator->Release();
	pCommandQueue->Release();
	fence->Release();
	device->Release();
	factory->Release();
}
