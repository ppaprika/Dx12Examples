#define WIN32_LEAN_AND_MEAN

#include <DirectXMath.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>

#include "Helpers.h"

#include <dxgidebug.h>
#include <float.h>
#include <Shlwapi.h>

#include "Camera.h"
#include "IndexBufferView.h"
#include "UploadBuffer.h"
#include "VertexBufferView.h"

using namespace DirectX;


XMMATRIX mvp_matrix;

struct VertexPosColor
{
    XMFLOAT3 Position;
    XMFLOAT3 Color;
    XMFLOAT2 TexCoord;
};

VertexPosColor vertexes_[8] = {
{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT2(0, 1)}, // 0
{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f),  XMFLOAT2(0, 0) }, // 1
{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1, 0) }, // 2
{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) , XMFLOAT2(1, 1)}, // 3
{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0, 1) }, // 4
{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) , XMFLOAT2(0, 0)}, // 5
{XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) , XMFLOAT2(1, 0)}, // 6
{XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) , XMFLOAT2(1, 1)}  // 7
};

WORD indexes_[36] =
{
    0, 1, 2, 0, 2, 3,
    4, 6, 5, 4, 7, 6,
    4, 5, 1, 4, 1, 0,
    3, 2, 6, 3, 6, 7,
    1, 5, 6, 1, 6, 2,
    4, 0, 3, 4, 3, 7
};

constexpr size_t back_buffer_num = 3;
size_t current_back_buffer_index = 0;


UINT64 fence_value = 0;



void ReportLiveObjects()
{
	IDXGIDebug1* dxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

	dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	dxgiDebug->Release();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    int retCode = 0;

    // Set the working directory to the path of the executable.
    WCHAR path[MAX_PATH];
    HMODULE hModule = GetModuleHandleW(nullptr);
    if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
    {
        PathRemoveFileSpecW(path);
        SetCurrentDirectoryW(path);
    }

    // register window
    constexpr wchar_t CLASS_NAME[] = L"Dx12Example";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // create window
    HWND hwnd = CreateWindowEx(
		0,
        CLASS_NAME,
        L"Window_1",
        WS_OVERLAPPEDWINDOW,
        600, 600, 600, 600,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if(hwnd == nullptr)
    {
        ShowLastError();
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    Camera camera;

    // enable debug layer
    ComPtr<ID3D12Debug> debug_controller;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)));
    debug_controller->EnableDebugLayer();

    // create dx device
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIFactory2> factory;
    CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory));

    {
        UINT index = 0;
        UINT max_capacity = 0;
        ComPtr<IDXGIAdapter> temp_adapter;
        while (factory->EnumAdapters(index, &temp_adapter) == S_OK)
        {
            DXGI_ADAPTER_DESC adapter_desc;
            temp_adapter->GetDesc(&adapter_desc);
            if (adapter_desc.DedicatedVideoMemory > max_capacity)
            {
                max_capacity = adapter_desc.DedicatedVideoMemory;
                adapter = temp_adapter;
            }
            index++;
        }
    }

    ComPtr<ID3D12Device> device;
    ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&device)));

    // upload vertex and index view
    std::shared_ptr<UploadBuffer> upload_buffer = std::make_shared<UploadBuffer>(device);
    VertexBufferView vertex_buffer_view;
    IndexBufferView index_buffer_view;
    vertex_buffer_view.Init(sizeof(VertexPosColor) * _countof(vertexes_), 64, sizeof(VertexPosColor), vertexes_, upload_buffer);
    index_buffer_view.Init(sizeof(WORD) * _countof(indexes_), 0, DXGI_FORMAT_R16_UINT, indexes_, upload_buffer);

    // create command allocator
    ComPtr<ID3D12CommandAllocator> allocators[back_buffer_num];
    for(size_t i = 0; i < back_buffer_num; ++i)
    {
        ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocators[i])));
    }

    // create command lists
    ComPtr<ID3D12CommandList> command_lists;
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocators[current_back_buffer_index].Get(), nullptr, IID_PPV_ARGS(&command_lists)));

    // create command queue
    ComPtr<ID3D12CommandQueue> command_queue;
    {
        D3D12_COMMAND_QUEUE_DESC queue_desc = {};
        queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
        ThrowIfFailed(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)));
    }

    // create fence
    ComPtr<ID3D12Fence> fence;
    ThrowIfFailed(device->CreateFence(fence_value, D3D12_FENCE_FLAG_NONE,  IID_PPV_ARGS(&fence)));

    // create swap chain
    ComPtr<IDXGISwapChain3> swap_chain;
    {
        ComPtr<IDXGISwapChain> temp_chain;

        DXGI_MODE_DESC mode_desc = {};
        mode_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

        DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
        swap_chain_desc.BufferCount = back_buffer_num;
        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.OutputWindow = hwnd;
        swap_chain_desc.Windowed = true;
        swap_chain_desc.BufferDesc = mode_desc;
        swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_desc.SampleDesc = { 1, 0 };

        ThrowIfFailed(factory->CreateSwapChain(command_queue.Get(), &swap_chain_desc, &temp_chain));
        temp_chain.As(&swap_chain);
    }





    MSG msg = {};
    while(GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
        // Add other message handlers here if needed
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}