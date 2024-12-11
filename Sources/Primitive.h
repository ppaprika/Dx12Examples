#pragma once
#include "IndexBufferView.h"
#include "TextureLoader.h"
#include "VertexBufferView.h"


class Window;

class Primitive
{
public:
	Primitive();

	//virtual void Draw(std::shared_ptr<DirectCommandList> commandList, std::shared_ptr<Window> window);

	const VertexBufferView* GetVertexBufferView() const { return &vertex_buffer_view_; }
	const IndexBufferView* GetIndexBufferView() const { return &index_buffer_view_; }


	virtual void PrepareForDrawing(ComPtr<ID3D12GraphicsCommandList2> commandList);

	virtual void SetRootParams(ComPtr<ID3D12GraphicsCommandList2> commandList);

	virtual size_t GetIndexCount() const = 0;

	virtual void Draw(ComPtr<ID3D12GraphicsCommandList2> commandList) = 0;
protected:
	VertexBufferView vertex_buffer_view_;
	IndexBufferView index_buffer_view_;

	TextureLoader::TextureResource texture_ = {};
	std::vector<TextureLoader::TextureResource> textures_;

	ComPtr<ID3D12DescriptorHeap> srv_desc_heap_;

	ComPtr<ID3D12RootSignature> root_signature_;
	ComPtr<ID3D12PipelineState> pipeline_state_;

	ComPtr<ID3DBlob> vertex_shader_;
	ComPtr<ID3DBlob> pixel_shader_;
};
