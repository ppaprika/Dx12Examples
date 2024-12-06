#pragma once
#include <dxgi.h>
#include <dxgi1_4.h>
#include <vector>
#include <D3DX12/d3d12.h>
#include <D3DX12/d3dx12_core.h>
#include "Game.h"

class DirectCommandList;
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

	std::shared_ptr<DirectCommandList> command_list;
};

class Window
{
public:
	Window(const CreateWindowParams& Params);
	virtual ~Window();

	HWND GetWindow() const { return window_; }

	int GetWidth() const { return width_; }

	int GetHeight() const { return height_; }


	void ResizeWindow(int width, int height);

	ComPtr<ID3D12Resource> GetCurrentBackBuffer();

	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle();

	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle();

	UINT num_of_back_buffers;
	UINT current_back_buffer;

	ComPtr<IDXGISwapChain3> swap_chain;

	std::vector<ComPtr<ID3D12Resource>> back_buffers;
	ComPtr<ID3D12DescriptorHeap> rtv_heap;

	ComPtr<ID3D12Resource> depth_buffer;
	ComPtr<ID3D12DescriptorHeap> dsv_heap;

	CD3DX12_VIEWPORT viewport;
	D3D12_RECT d3d12_rect;

private:
	int width_;
	int height_;
	HWND window_;
	std::weak_ptr<DirectCommandList> owner_;

	void InitViewportAndRect();
	void CreateDsvHeap();
	void ResizeDepthBuffer();

	void CreateSwapChain();
	static bool UpdateRenderTarget(ComPtr<ID3D12Device> device, ComPtr<IDXGISwapChain> swapChain, std::vector<ComPtr<ID3D12Resource>>& resource, UINT bufferNum, ComPtr<ID3D12DescriptorHeap> heap, D3D12_DESCRIPTOR_HEAP_TYPE type);
	void CreateRtvHeap();
};
