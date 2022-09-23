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
	// vertex data
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{ NULL };
	std::vector<D3D12_INPUT_ELEMENT_DESC> vertexLayout;

	int vertexCount{ 0 };
	UINT vertexStride{ 0 };

	size_t vertexDataSize{ 0 };
	void* vertexData{ nullptr };

	// index data
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW indexBufferView{ NULL };

	int indexCount;

	size_t indexDataSize{ 0 };
	void* indexData{ nullptr };
};

