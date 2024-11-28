#pragma once
#include <D3DX12/d3d12.h>
#include <D3DX12/d3dx12_barriers.h>
#include <D3DX12/d3dx12_resource_helpers.h>
#include <wrl/client.h>

#include "DirectXTex.h"


using Microsoft::WRL::ComPtr;
using namespace DirectX;

class TextureLoader
{
public:
	struct TextureResource
	{
		ComPtr<ID3D12Resource> texture;
		ComPtr<ID3D12Resource> upload_heap;
		D3D12_CPU_DESCRIPTOR_HANDLE srv_handle;
	};

	static HRESULT LoadTextureFromFile
	(
		ComPtr<ID3D12Device> device,
		ComPtr<ID3D12GraphicsCommandList> commandList,
		const wchar_t* filename,
		TextureResource& outResource,
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle
	)
	{
		ScratchImage scratchImage;
		HRESULT hr = LoadFromWICFile(
			filename,
			WIC_FLAGS_NONE,
			nullptr,
			scratchImage
		);

		if (FAILED(hr)) return hr;

		const Image* img = scratchImage.GetImage(0, 0, 0);

		// for texture
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Width = img->width;
		textureDesc.Height = img->height;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.MipLevels = 1;
		textureDesc.Format = img->format;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 1;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES heapProps = {
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			0, 0
		};

		hr = device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&outResource.texture)
		);

		if (FAILED(hr)) return hr;

		// for upload buffer
		size_t uploadBufferSize;
		device->GetCopyableFootprints(
			&textureDesc,
			0, 1, 0,
			nullptr,
			nullptr,
			nullptr,
			&uploadBufferSize
		);

		D3D12_HEAP_PROPERTIES uploadHeapProps = {
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			0,0
		};

		D3D12_RESOURCE_DESC uploadBufferDesc = {
			D3D12_RESOURCE_DIMENSION_BUFFER,
			0,
			uploadBufferSize,
			1,
			1,
			1,
			DXGI_FORMAT_UNKNOWN,
			{1, 0},
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			D3D12_RESOURCE_FLAG_NONE
		};

		hr = device->CreateCommittedResource(
			&uploadHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&uploadBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&outResource.upload_heap)
		);

		// copy data to upload buffer
		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = img->pixels;
		textureData.RowPitch = img->rowPitch;
		textureData.SlicePitch = img->slicePitch;

		UpdateSubresources(
			commandList.Get(),
			outResource.texture.Get(),
			outResource.upload_heap.Get(),
			0, 0, 1,
			&textureData
		);

		// texture -> shader resource
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			outResource.texture.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);

		commandList->ResourceBarrier(1, &barrier);

		// create SRV
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		device->CreateShaderResourceView(
			outResource.texture.Get(),
			&srvDesc,
			srvHandle
		);

		outResource.srv_handle = srvHandle;

		return S_OK;
	}
};