#include "rendering/renderTarget.h"
#include "helper.h"

#include "rendering\renderBackend.h"

void RenderTarget::transitionRtvTo(_In_ ID3D12GraphicsCommandList* aCommandList, D3D12_RESOURCE_STATES aAfterState)
{
    TransitionResource(aCommandList, rtvResource.Get(), rtvState, aAfterState);
    rtvState = aAfterState;
}

void RenderTarget::transitionDsvTo(_In_ ID3D12GraphicsCommandList* aCommandList, D3D12_RESOURCE_STATES aAfterState)
{
    TransitionResource(aCommandList, dsvResource.Get(), dsvState, aAfterState);
    dsvState = aAfterState;
}

void RenderTarget::beginScene(_In_ ID3D12GraphicsCommandList* aCommandList)
{
    transitionRtvTo(aCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    transitionDsvTo(aCommandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void RenderTarget::endScene(_In_ ID3D12GraphicsCommandList * aCommandList)
{
    transitionRtvTo(aCommandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    transitionDsvTo(aCommandList, D3D12_RESOURCE_STATE_DEPTH_READ);
}

Texture* RenderTarget::getRtvTexture()
{
    TextureDesc textureDesc{};
    textureDesc.width = width;
    textureDesc.height = height;
    textureDesc.bytesPerPixel = 4;
    textureDesc.data = nullptr;

    textureDesc.name = "default";

    return RenderBackend::getTextureFromRenderTarget(this, false);
}

Texture* RenderTarget::getDsvTexture()
{
    TextureDesc textureDesc{};
    textureDesc.width = width;
    textureDesc.height = height;
    textureDesc.bytesPerPixel = 4;
    textureDesc.data = nullptr;

    textureDesc.name = "default";

    return RenderBackend::getTextureFromRenderTarget(this, true);
}