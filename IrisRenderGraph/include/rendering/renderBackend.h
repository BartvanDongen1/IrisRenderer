#pragma once
#include "resources\resourceDescs.h"

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <wrl.h>
#include "d3dx12.h"

class PipelineObject;
class Mesh;
class ConstBuffer;
class Texture;
class Sampler;
class Drawable;
class Drawable2;
class RenderTarget;
class RenderPassGraph;
class DebugModelBuilder;
class Camera;

class RenderBackend
{
public:
	static bool loadPipeline();
	static void loadAssets();
	static void initImGui();

	static void shutdown();

	static void beginFrame();
	static void endFrame();
	//static void renderFrame();

	static void renderDebugModel(Camera* aCamera);
	static void renderImGui(int aFPS);

	static void presentRenderTarget(RenderTarget* aRenderTarget);

	static void setGraphForResources(RenderPassGraph* aGraph);

	//static Handle createDescriptorHeap(DescriptorHeapDesc aDesc);
	static RenderTarget* createRenderTarget(RenderTargetDesc aDesc);

	static PipelineObject* createPipelineObject(PipelineObjectDesc aDesc);
	static Mesh* createMesh(MeshDesc aDesc);

	static void clearRenderTarget(RenderTarget* aRenderTarget);
	static void beginScene(RenderTarget* aRenderTarget);
	static void endScene(RenderTarget* aRenderTarget);
	static void drawDrawable(Drawable* aDrawable);
	static void drawDrawable2(Drawable2* aDrawable);

	static ConstBuffer* initConstBuffer(ConstBufferDesc aDesc);
	static Texture* initTexture(TextureDesc aDesc);
	static Texture* getTextureFromRenderTarget(RenderTarget* aRenderTarget = nullptr, bool useDSV = false);
	static Sampler* initSampler(SamplerDesc aDesc);
};

