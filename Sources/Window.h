#pragma once
#include <dxgi.h>
#include <dxgi1_4.h>
#include <vector>
#include <D3DX12/d3d12.h>
#include <D3DX12/d3dx12_core.h>

#include "Game.h"

class CommandList;
using Microsoft::WRL::ComPtr;


// todo: change name to RunGameParams
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
	int numOfBackBuffers;
};

class Window
{
public:
	Window(std::shared_ptr<Game> Owner, const CreateWindowParams& Params);
	virtual ~Window();

	HWND GetWindow() { return _window; }
	int GetWidth() { return _width; }
	void SetWidth(int width) { _width = width; }
	int GetHeight() { return _height; }
	void SetHeight(int height) { _height = height; }


	void InitWindow();
	void InitViewportAndRect();
	void InitDepth();
	void ResizeDepthBuffer();

	void UpdateSize(int width, int height);

	void Flush();
public:
	static ComPtr<IDXGISwapChain> CreateSwapChain(HWND window, ComPtr<ID3D12CommandQueue> queue, int numOfBackBuffers);
	static bool UpdateRenderTarget(ComPtr<ID3D12Device> device, ComPtr<IDXGISwapChain> swapChain, std::vector<ComPtr<ID3D12Resource>>& resource, UINT bufferNum, ComPtr<ID3D12DescriptorHeap> heap, D3D12_DESCRIPTOR_HEAP_TYPE type, SIZE_T& heapSize);
	static ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device> device, UINT num, D3D12_DESCRIPTOR_HEAP_TYPE type);
	static ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device> device, UINT fenceValue);


	HWND _window;
	std::weak_ptr<Game> _owner;

public:

	UINT _numOfBackBuffers;
	UINT _currentBackBuffer;

	std::shared_ptr<CommandList> _commandList;
	ComPtr<IDXGISwapChain3> _swapChain;
	std::vector<ComPtr<ID3D12Resource>> _backBuffers;
	ComPtr<ID3D12DescriptorHeap> _descriptorHeap;
	SIZE_T _heapSize;

	ComPtr<ID3D12Resource> _depthBuffer;
	ComPtr<ID3D12DescriptorHeap> _dsvHeap;

	int _width;
	int _height;

	CD3DX12_VIEWPORT _viewport;
	D3D12_RECT _d3d12Rect;
};
