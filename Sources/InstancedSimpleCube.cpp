
#include "InstancedSimpleCube.h"

#include <d3dcompiler.h>
#include <D3DX12/d3dx12_pipeline_state_stream.h>
#include <D3DX12/d3dx12_root_signature.h>

#include "Application.h"
#include "Helpers.h"
#include "DirectCommandList.h"


InstancedSimpleCube::InstancedSimpleCube(std::shared_ptr<UploadBuffer> buffer,
                                         std::shared_ptr<class DirectCommandList> commandList)
{
	vertex_buffer_view_.Init(sizeof(vertexes_), 64, sizeof(VertexPosColor), vertexes_, buffer);
	index_buffer_view_.Init(sizeof(indexes_), 0, DXGI_FORMAT_R16_UINT, indexes_, buffer);

	ThrowIfFailed(D3DReadFileToBlob(L"InstancedVertexShader.cso", &vertex_shader_));
	ThrowIfFailed(D3DReadFileToBlob(L"PixelShader.cso", &pixel_shader_));

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

	// create root signature
	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[1] = {};
	descriptorRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE);

	CD3DX12_ROOT_PARAMETER1 rootParameter[3] = {};
	rootParameter[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameter[1].InitAsDescriptorTable(1, &descriptorRange[0], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[2].InitAsConstants(sizeof(XMMATRIX) / 4, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription = {};
	rootSignatureDescription.Init_1_1(_countof(rootParameter), rootParameter, 1, &samplerDesc, rootSignatureFlags);

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
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

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
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(Application::GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srv_desc_heap_)));
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srv_desc_heap_->GetCPUDescriptorHandleForHeapStart();
	TextureLoader::LoadTextureFromFile(Application::GetDevice(), commandList->GetCommandList(), L"../../resources/Texture.jpg", texture_, srvHandle);
	commandList->SingleAndWait();

	// init mvp matrix
	mvp_matrix = DirectX::XMMatrixIdentity();

	for(int i = 0; i < _countof(instance_pos_); ++i)
	{
		instance_pos_[i] = DirectX::XMMatrixTranslation(i * 0.5f - 1, 0, 0);
	}
}

void InstancedSimpleCube::SetRootParams(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	Primitive::SetRootParams(commandList);

}

void InstancedSimpleCube::Draw(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvp_matrix, 0);
	commandList->SetDescriptorHeaps(1, srv_desc_heap_.GetAddressOf());
	commandList->SetGraphicsRootDescriptorTable(1, srv_desc_heap_->GetGPUDescriptorHandleForHeapStart());
	for(int i = 0; i < _countof(instance_pos_); ++i)
	{
		commandList->SetGraphicsRoot32BitConstants(2, sizeof(XMMATRIX) / 4, &instance_pos_[i], 0);
		commandList->DrawIndexedInstanced(GetIndexCount(), 1, 0, 0, 0);
	}
}
