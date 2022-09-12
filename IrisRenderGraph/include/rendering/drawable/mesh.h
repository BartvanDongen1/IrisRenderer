#pragma once

#include <vector>
#include <d3d12.h>
#include <wrl.h>

class Mesh
{
	friend class RenderBackend;
public:
	Mesh() {};
	~Mesh() {};

	void bind(ID3D12GraphicsCommandList* aCommandList);
	int getVertexCount() { return vertexCount; };
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{ NULL };
	std::vector<D3D12_INPUT_ELEMENT_DESC> vertexLayout;

	int vertexCount{ 0 };
	UINT vertexStride{ 0 };

	size_t dataSize{ 0 };
	void* data{ nullptr };
};

