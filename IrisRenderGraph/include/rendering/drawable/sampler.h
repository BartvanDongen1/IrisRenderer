#pragma once
#include "descriptor.h"
#include "rendering/d3dx12.h"

class Sampler : public Descriptor
{
	friend class RenderBackend;
public:
	Sampler() {};
	~Sampler() {};

	void update(void* aData, size_t aSize) override;

private:
	D3D12_SAMPLER_DESC description;
};

