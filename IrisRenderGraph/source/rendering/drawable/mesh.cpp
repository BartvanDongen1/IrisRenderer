#include "rendering/drawable/mesh.h"

void Mesh::bind(ID3D12GraphicsCommandList* aCommandList)
{
	aCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
}
