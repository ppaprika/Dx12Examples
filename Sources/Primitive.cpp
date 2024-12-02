#include "Primitive.h"



Primitive::Primitive()
{

}

void Primitive::PrepareForDrawing(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	commandList->SetPipelineState(pipeline_state_.Get());
	commandList->SetGraphicsRootSignature(root_signature_.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetIndexBuffer(index_buffer_view_.GetIndexBufferView());
	commandList->IASetVertexBuffers(0, 1, vertex_buffer_view_.GetVertexBufferView());
}

void Primitive::SetRootParams(ComPtr<ID3D12GraphicsCommandList2> commandList)
{

}

//void Primitive::Draw(std::shared_ptr<DirectCommandList> commandList, std::shared_ptr<Window> window)
//{
//
//}
