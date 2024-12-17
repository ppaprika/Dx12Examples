
#include "TextureLoader.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgi1_3.h>
#include <dxgi1_4.h>

#include <dxgidebug.h>
#include <float.h>
#include <Shlwapi.h>
#include <D3DX12/d3dx12_core.h>
#include <D3DX12/d3dx12_root_signature.h>

#include "Helpers.h"
#include "UploadBuffer.h"
#include "Camera.h"
#include "IndexBufferView.h"
#include "VertexBufferView.h"



#ifdef TestIncludes
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    return 0;
}
#else


using namespace DirectX;

constexpr UINT window_width = 600;
constexpr UINT window_height = 600;


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
        600, 600, window_width, window_height,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (hwnd == nullptr)
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
    for (size_t i = 0; i < back_buffer_num; ++i)
    {
        ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocators[i])));
    }

    // create command lists
    ComPtr<ID3D12GraphicsCommandList> command_lists;
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
    ThrowIfFailed(device->CreateFence(fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

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

    // we use two-pass rendering, so, we have:
    // 3 final rtv
    // 1 intermediate rtv and 1 srv created on same resource
    // 1 dsv

    // create final render target view descriptor heap
    ComPtr<ID3D12DescriptorHeap> final_rtv_desc_heap;
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
        rtv_heap_desc.NumDescriptors = back_buffer_num;
        rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&final_rtv_desc_heap));
    }

    // final render target view
    ComPtr<ID3D12Resource> final_rt_resources[back_buffer_num];
    {
        auto handle = final_rtv_desc_heap->GetCPUDescriptorHandleForHeapStart();
        size_t descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        for (size_t i = 0; i < back_buffer_num; ++i)
        {
            ComPtr<ID3D12Resource> resource;
            swap_chain->GetBuffer(i, IID_PPV_ARGS(&resource));
            D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
            rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            device->CreateRenderTargetView(resource.Get(), &rtv_desc, handle);
            handle.ptr += descriptor_size;
            final_rt_resources[i] = resource;
        }
    }

    // intermediate resource
    ComPtr<ID3D12Resource> intermediate_resource;
    {
        CD3DX12_HEAP_PROPERTIES heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resource_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, window_width, window_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

        device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&intermediate_resource));
    }


    ComPtr<ID3D12DescriptorHeap> intermediate_rtv_desc_heap;
    ComPtr<ID3D12DescriptorHeap> intermediate_srv_desc_heap;
    {
        // intermediate rtv
        D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
        rtv_heap_desc.NumDescriptors = 1;
        rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&intermediate_rtv_desc_heap));

        D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
        rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        device->CreateRenderTargetView(intermediate_resource.Get(), &rtv_desc, intermediate_rtv_desc_heap->GetCPUDescriptorHandleForHeapStart());

        // intermediate srv
        D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
        srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srv_heap_desc.NumDescriptors = 1;
        srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&intermediate_srv_desc_heap));

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Texture2D = { 0, 1 };
        device->CreateShaderResourceView(intermediate_resource.Get(), &srv_desc, intermediate_srv_desc_heap->GetCPUDescriptorHandleForHeapStart());
    }

    // dsv resource
    ComPtr<ID3D12Resource> dsv_resource;
    {
        CD3DX12_HEAP_PROPERTIES heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC res_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, window_width, window_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
        D3D12_CLEAR_VALUE clear_value = {};
        clear_value.Format = DXGI_FORMAT_D32_FLOAT;
        clear_value.DepthStencil = { 1, 0 };
        device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &res_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, IID_PPV_ARGS(&dsv_resource));
    }

    // dsv
    ComPtr<ID3D12DescriptorHeap> dsv_desc_heap;
    {
        D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
        heap_desc.NumDescriptors = 1;
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&dsv_desc_heap));

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
        dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
        dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsv_desc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
        dsv_desc.Texture2D.MipSlice = 0;
        device->CreateDepthStencilView(dsv_resource.Get(), &dsv_desc, dsv_desc_heap->GetCPUDescriptorHandleForHeapStart());
    }

    // we need two PSOs, each for one rendering pass
    // intermediate pass
    ComPtr<ID3D12RootSignature> intermediate_root_signature;
    ComPtr<ID3D12PipelineState> intermediate_pso;
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

        CD3DX12_ROOT_PARAMETER1 root_parameters[2];
        root_parameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        root_parameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC sampler_desc;
        sampler_desc.Init(0);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc = {};
        root_signature_desc.Init_1_1(_countof(root_parameters), &root_parameters[0], 1, &sampler_desc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        ComPtr<ID3DBlob> root_signature_blob;
        ComPtr<ID3DBlob> error_blob;
        D3DX12SerializeVersionedRootSignature(&root_signature_desc, featureData.HighestVersion, &root_signature_blob, &error_blob);
        device->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&intermediate_root_signature));

        ComPtr<ID3DBlob> intermediate_vertex_shader;
        ComPtr<ID3DBlob> intermediate_pixel_shader;

        D3DReadFileToBlob(L"VertexShader.cso", &intermediate_vertex_shader);
        D3DReadFileToBlob(L"PixelBlendShader.cso", &intermediate_pixel_shader);

        D3D12_INPUT_ELEMENT_DESC intermediate_input_elements[] = {
            {"MYPOSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"MYCOLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"MYTEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
        pso_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        pso_desc.VS = CD3DX12_SHADER_BYTECODE(intermediate_vertex_shader.Get());
        pso_desc.PS = CD3DX12_SHADER_BYTECODE(intermediate_pixel_shader.Get());
        pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso_desc.NumRenderTargets = 1;
        pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pso_desc.pRootSignature = intermediate_root_signature.Get();
        pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());
        pso_desc.SampleDesc = { 1, 0 };
        pso_desc.InputLayout = { intermediate_input_elements, _countof(intermediate_input_elements) };

        device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&intermediate_pso));
    }


    // todo: test single pass rendering, make sure my pipeline is constructed correctly!
    // Load Texture
    ComPtr<ID3D12DescriptorHeap> rsv_des_heap;
    {
        D3D12_DESCRIPTOR_HEAP_DESC rsv_des_heap_desc = {};
        rsv_des_heap_desc.NumDescriptors = 1;
        rsv_des_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        device->CreateDescriptorHeap(&rsv_des_heap_desc, IID_PPV_ARGS(&rsv_des_heap));
    }
    TextureLoader::TextureResource texture_resource;
    TextureLoader::LoadTextureFromFile(device, command_lists, L"resources/Texture.png", texture_resource, rsv_des_heap->GetCPUDescriptorHandleForHeapStart());

    {
        command_lists->Close();
        ID3D12CommandList* lists[1] = {command_lists.Get()};
        command_queue->ExecuteCommandLists(1, lists);
        command_queue->Signal(fence.Get(), fence_value);

        fence_value++;
    }
    




    // final pass






    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Render()
{

}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_PAINT:
        Render();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
        // Add other message handlers here if needed
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif

