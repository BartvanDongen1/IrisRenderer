#include "rendering/renderGraph/passes/fullScreenPass.h"
#include "rendering\drawable\drawable.h"
#include "engine\resourceLoader.h"
#include "rendering\renderGraph\job.h"
#include "rendering/renderGraph/renderPassGraph.h"
#include "rendering\renderBackend.h"

FullScreenPass::FullScreenPass(const char* aName)
	:
	BasePass(aName)
{

}

FullScreenPass::~FullScreenPass()
{

}

void FullScreenPass::init()
{
	const char* input = resources.find("input")->second.second;

	// setup quad for post process
	{
		Drawable* myModel = new Drawable();

		PipelineObjectDesc myDesc;

		myDesc.vertexShader = L"resources/shaders/quadPost.hlsl";
		myDesc.pixelShader = L"resources/shaders/quadPost.hlsl";

		myDesc.vertexLayout.push_back(vertexType::Position3);
		myDesc.vertexLayout.push_back(vertexType::Normal3);
		myDesc.vertexLayout.push_back(vertexType::TexCoord2);

		TextureDesc myTextureDesc{};
		myTextureDesc.renderTargetName = input;

		myDesc.textures.push_back(myTextureDesc);

		myDesc.samplers.push_back({ "sampler" });

		myModel->setPipeline(myDesc);

		myModel->setMesh(ResourceLoader::getMesh("resources/meshes/TheQuad.obj"));

		fullscreenJob = new Job(myModel, "quad");
	}
}

void FullScreenPass::execute()
{
	RenderBackend::beginScene(resources.find("output")->second.first);

	fullscreenJob->execute();

	RenderBackend::endScene(resources.find("output")->second.first);
}
