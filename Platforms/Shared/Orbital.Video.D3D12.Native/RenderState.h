#pragma once
#include "Device.h"

struct RenderState
{
	Device* device;
	ID3D12PipelineState* state;
	ID3D12RootSignature* shaderEffectSignature;
	D3D_PRIMITIVE_TOPOLOGY topology;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
};