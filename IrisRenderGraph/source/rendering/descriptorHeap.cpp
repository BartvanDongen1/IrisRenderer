#include "rendering/descriptorHeap.h"
#include "helper.h"

void DescriptorHeap::init(ID3D12Device* aDevice, int aDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE aType, D3D12_DESCRIPTOR_HEAP_FLAGS aFlags)
{
	device = aDevice;
    descriptorCount = aDescriptorCount;

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = aDescriptorCount;
    heapDesc.Type = aType;
    heapDesc.Flags = aFlags;
    ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap)));

    descriptorSize = device->GetDescriptorHandleIncrementSize(aType);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::getCPUHandle(UINT aIndex)
{
    assert(aIndex < descriptorCount);

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(heap->GetCPUDescriptorHandleForHeapStart(), aIndex, descriptorSize);
    return handle;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::getGPUHandle(UINT aIndex)
{
    assert(aIndex < descriptorCount);

    CD3DX12_GPU_DESCRIPTOR_HANDLE handle(heap->GetGPUDescriptorHandleForHeapStart(), aIndex, descriptorSize);

    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::getDescHandle()
{
    return heap.Get()->GetCPUDescriptorHandleForHeapStart();
}
