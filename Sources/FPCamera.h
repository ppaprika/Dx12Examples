#pragma once
#include "Game.h"


class FPCamera : public Game
{



private:

	std::vector<ComPtr<ID3D12PipelineState>> pipeline_states_;
	std::vector<ComPtr<ID3D12RootSignature>> root_signatures_;
};
