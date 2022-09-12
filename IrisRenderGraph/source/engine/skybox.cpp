#include "engine/skybox.h"
#include "rendering/drawable/drawable.h"
#include "engine/resourceLoader.h"
#include "rendering/renderGraph/job.h"
#include "engine/camera.h"

void Skybox::init()
{
	drawable = new Drawable();
	skyboxBuffer = new SkyboxBuffer();

	PipelineObjectDesc myDesc;

	myDesc.vertexShader = L"resources/shaders/skybox.hlsl";
	myDesc.pixelShader = L"resources/shaders/skybox.hlsl";

	myDesc.vertexLayout.push_back(vertexType::Position3);
	myDesc.vertexLayout.push_back(vertexType::Normal3);
	myDesc.vertexLayout.push_back(vertexType::TexCoord2);

	ConstBufferDesc myConstBufferDesc;
	myConstBufferDesc.name = "constbuffer";
	myConstBufferDesc.data = skyboxBuffer;
	myConstBufferDesc.size = sizeof(SkyboxBuffer);

	myDesc.constBuffers.push_back(myConstBufferDesc);

	myDesc.textures.push_back(ResourceLoader::getTexture("resources/textures/skybox.png"));;

	myDesc.samplers.push_back({ "sampler" });

	drawable->setPipeline(myDesc);

	drawable->setMesh(ResourceLoader::getMesh("resources/meshes/skybox.obj"));
}

void Skybox::update()
{
	skyboxBuffer->viewProjectionMatrix = camera->getProjectionMatrix() * glm::mat4(glm::mat3(camera->getViewMatrix()));
}

void Skybox::shutdown()
{
	delete drawable;
	delete skyboxBuffer;
}

Job* Skybox::getJob()
{
	Job* myJob = new Job(drawable, "skybox");

	BufferUpdate myUpdate;
	myUpdate.bufferName = "constbuffer";
	myUpdate.data = skyboxBuffer;
	myUpdate.dataSize = sizeof(SkyboxBuffer);

	myJob->addBufferUpdate(myUpdate);

    return myJob;
}
