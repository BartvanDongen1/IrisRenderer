#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>

class Descriptor
{
	friend class RenderBackend;
public:
	Descriptor() {};
	~Descriptor() {};

	virtual void update(void* aData, size_t aSize) = 0;

protected:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	UINT descriptorSize{ 0 };
};

