#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "d3dx12.h"

class DescriptorHeap
{
	friend class RenderBackend;
public:
	DescriptorHeap() {};

	void init(ID3D12Device* aDevice, int aDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE aType, D3D12_DESCRIPTOR_HEAP_FLAGS aFlags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	CD3DX12_CPU_DESCRIPTOR_HANDLE getCPUHandle(UINT aIndex);
	CD3DX12_GPU_DESCRIPTOR_HANDLE getGPUHandle(UINT aIndex);

	D3D12_CPU_DESCRIPTOR_HANDLE getDescHandle();

private:

	Microsoft::WRL::ComPtr<ID3D12Device>                device{ };

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        heap{};
	UINT												descriptorSize{ 0 };

	UINT												descriptorCount{ 0 };
};

