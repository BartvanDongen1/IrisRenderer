#pragma once
#include <d3d12.h>
#include "rendering/d3dx12.h"

#include "descriptor.h"

class ConstBuffer : public Descriptor
{
	friend class RenderBackend;
public:
	ConstBuffer() {};
	~ConstBuffer() {};

	void update(void* aData, size_t aSize) override;

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer;
	D3D12_CONSTANT_BUFFER_VIEW_DESC description{ 0 };

	UINT8* pCbvDataBegin = nullptr;
	void* constantBufferData = nullptr;
	size_t constantBufferSize{ 0 };
};

