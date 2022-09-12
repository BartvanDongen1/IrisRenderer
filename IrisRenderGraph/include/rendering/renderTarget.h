#pragma once
#include "rendering/drawable/texture.h"

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl.h>

class RenderTarget
{
	friend class RenderBackend;
public:
	RenderTarget() {};
	virtual ~RenderTarget() {};

	void transitionRtvTo(_In_ ID3D12GraphicsCommandList* aCommandList, D3D12_RESOURCE_STATES aAfterState);
	void transitionDsvTo(_In_ ID3D12GraphicsCommandList* aCommandList, D3D12_RESOURCE_STATES aAfterState);

	void beginScene(_In_ ID3D12GraphicsCommandList* aCommandList);
	void endScene(_In_ ID3D12GraphicsCommandList* aCommandList);

	Texture* getRtvTexture();
	Texture* getDsvTexture();

private:
	Microsoft::WRL::ComPtr<ID3D12Device> device{ };

	Microsoft::WRL::ComPtr<ID3D12Resource> rtvResource{ };
	Microsoft::WRL::ComPtr<ID3D12Resource> dsvResource{ };

	D3D12_RESOURCE_STATES rtvState{ D3D12_RESOURCE_STATE_COMMON };
	D3D12_RESOURCE_STATES dsvState{ D3D12_RESOURCE_STATE_COMMON };

	DXGI_FORMAT rtvFormat{ };
	DXGI_FORMAT dsvFormat{ };

	CD3DX12_VIEWPORT viewport{ };
	CD3DX12_RECT scissorRect{ };

	size_t width{ 0 };
	size_t height{ 0 };

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap{};
	UINT rtvHeapSize{ 0 };

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap{};
	UINT dsvHeapSize{ 0 };
};