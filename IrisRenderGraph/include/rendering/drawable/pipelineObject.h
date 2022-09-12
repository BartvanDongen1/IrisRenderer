#pragma once
#include <vector>
#include <unordered_map>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>

class Descriptor;

struct DescriptorInfo
{
	D3D12_DESCRIPTOR_HEAP_TYPE type;
	UINT index;
};

class PipelineObject
{
	friend class RenderBackend;
public:
	PipelineObject() {};
	~PipelineObject() {};

	std::unordered_map<const char*, Descriptor*> descriptors;
private:

	//directx variables
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

	// descriptor heaps
	//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> cbvSrvUavHeap;
	//UINT cbvSrvUavDescriptorSize{ 0 };
	int cbvSrvUavCount{ 0 };

	//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> samplerHeap;
	//UINT samplerDescriptorSize{ 0 };
	int samplerCount{ 0 };

	std::vector<DescriptorInfo> descriptorInfo;

	// shaders
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;
};

