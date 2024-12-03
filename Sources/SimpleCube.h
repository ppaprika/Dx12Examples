#pragma once


#include "Primitive.h"

class DirectCommandList;

class SimpleCube : public Primitive
{
public:
	size_t GetIndexCount() const override { return _countof(indexes_); }

	SimpleCube(std::shared_ptr<UploadBuffer> buffer, std::shared_ptr<DirectCommandList> commandList);

	void SetRootParams(ComPtr<ID3D12GraphicsCommandList2> commandList) override;

	XMMATRIX mvp_matrix;
	
private:

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
	{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) , XMFLOAT2(1, 1)}  // 7
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
};
