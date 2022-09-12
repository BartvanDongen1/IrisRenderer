#pragma once
#include "descriptor.h"

#include "rendering/d3dx12.h"

class Texture : public Descriptor
{
	friend class RenderBackend;
public:
	Texture() {};
	~Texture() {};

	void update(void* aData, size_t aSize) override;

protected:

	Microsoft::WRL::ComPtr<ID3D12Resource> texture;
	D3D12_SHADER_RESOURCE_VIEW_DESC description{};

	void* textureData{ 0 };

	int textureWidth{ 0 };
	int textureHeight{ 0 };
	int bytesPerPixel{ 0 };
};

