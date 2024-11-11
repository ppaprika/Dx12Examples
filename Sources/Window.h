#pragma once
#include <dxgi.h>
#include <dxgi1_4.h>

#include "CommandList.h"
#include "Game.h"

using Microsoft::WRL::ComPtr;

struct CreateWindowParams
{
	WNDPROC winProc;
	HINSTANCE hInstance;
	LPCWSTR wndClassName;
	LPCWSTR wndName;
	DWORD dwStyle;
	int x;
	int y;
	int nWidth;
	int nHeight;
	int nCmdShow;

	Game* owner;
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12CommandQueue> queue;
	int numOfBackBuffers;
};

class Window
{
public:
	Window(CreateWindowParams& Params);
	virtual ~Window();

	HWND GetWindow() { return _window; }
private:
	static ComPtr<IDXGISwapChain> CreateSwapChain(HWND window, ComPtr<ID3D12CommandQueue> queue, int numOfBackBuffers);
	static bool UpdateRenderTarget(ComPtr<ID3D12Device> device, ComPtr<IDXGISwapChain> swapChain, std::vector<ComPtr<ID3D12Resource>>& resource, UINT bufferNum, ComPtr<ID3D12DescriptorHeap> heap, D3D12_DESCRIPTOR_HEAP_TYPE type, SIZE_T& heapSize);
	static ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device> device, UINT num, D3D12_DESCRIPTOR_HEAP_TYPE type);


	HWND _window;
	Game* _owner;

public:
	ComPtr<IDXGISwapChain3> _swapChain;
	std::vector<ComPtr<ID3D12Resource>> _backBuffers;
	ComPtr<ID3D12DescriptorHeap> _descriptorHeap;
	SIZE_T _heapSize;
};
