


#include "BlendCube.h"

#include <d3dcompiler.h>
#include <D3DX12/d3dx12_pipeline_state_stream.h>
#include <D3DX12/d3dx12_root_signature.h>

#include "Application.h"
#include "Helpers.h"
#include "DirectCommandList.h"

BlendCube::BlendCube(std::shared_ptr<UploadBuffer> buffer, std::shared_ptr<DirectCommandList> commandList)
{
	vertex_buffer_view_.Init(sizeof(vertexes_), 64, sizeof(VertexPosColor), vertexes_, buffer);
	index_buffer_view_.Init(sizeof(indexes_), 0, DXGI_FORMAT_R16_UINT, indexes_, buffer);

	ThrowIfFailed(D3DReadFileToBlob(L"VertexShader.cso", &vertex_shader_));
	ThrowIfFailed(D3DReadFileToBlob(L"PixelBlendShader.cso", &pixel_shader_));

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
	{"MYPOSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"MYCOLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"MYTEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(Application::GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	CD3DX12_DESCRIPTOR_RANGE1 range[1];
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);

	CD3DX12_ROOT_PARAMETER1 root_parameter1[3];
	root_parameter1[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	root_parameter1[1].InitAsDescriptorTable(1, &range[0], D3D12_SHADER_VISIBILITY_PIXEL);
	// indicator of rendering texture 0 or 1
	root_parameter1[2].InitAsConstants(sizeof(uint32_t) / 4, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	D3D12_STATIC_SAMPLER_DESC sampler_desc = {};
	sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler_desc.MipLODBias = 0.0f;
	sampler_desc.MaxAnisotropy = 1;
	sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
	sampler_desc.ShaderRegister = 0;
	sampler_desc.RegisterSpace = 0;
	sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription = {};
	rootSignatureDescription.Init_1_1(_countof(root_parameter1), root_parameter1, 1, &sampler_desc, rootSignatureFlags);

	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob));

	ThrowIfFailed(Application::GetDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&root_signature_)));

	// create PSO
	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopology;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineStateStream;

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 2;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvFormats.RTFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipelineStateStream.pRootSignature = root_signature_.Get();
	pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
	pipelineStateStream.PrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertex_shader_.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixel_shader_.Get());
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.RTVFormats = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {};
	pipelineStateStreamDesc.pPipelineStateSubobjectStream = &pipelineStateStream;
	pipelineStateStreamDesc.SizeInBytes = sizeof(PipelineStateStream);

	ComPtr<ID3D12Device2> device2;
	ThrowIfFailed(Application::GetDevice().As(&device2));
	ThrowIfFailed(device2->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipeline_state_)));

	// load texture to srv
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 2;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(Application::GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srv_desc_heap_)));
	// first texture
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srv_desc_heap_->GetCPUDescriptorHandleForHeapStart();
	textures_.push_back(TextureLoader::TextureResource());
	TextureLoader::LoadTextureFromFile(Application::GetDevice(), commandList->GetCommandList(), L"../../resources/Texture.png", textures_[0], srvHandle);
	// second texture
	textures_.push_back(TextureLoader::TextureResource());
	srvHandle.ptr += Application::GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	TextureLoader::LoadTextureFromFile(Application::GetDevice(), commandList->GetCommandList(), L"../../resources/CrossLine.png", textures_[1], srvHandle);

	commandList->SingleAndWait();

	// init mvp matrix
	mvp_matrix = DirectX::XMMatrixIdentity();
}
