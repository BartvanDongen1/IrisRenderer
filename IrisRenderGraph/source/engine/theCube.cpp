#include "engine/theCube.h"
#include "rendering/drawable/drawable.h"
#include "rendering/drawable/pipelineObject.h"
#include "rendering/drawable/descriptor.h"
#include "engine/resourceLoader.h"
#include "rendering/renderGraph/job.h"
#include "rendering/drawable/bufferUpdate.h"
#include "engine/camera.h"

#include <glm/gtc/matrix_transform.hpp>

void TheCube::init()
{
	// setup for draw pass
	{
		drawable = new Drawable();

		{
			PipelineObjectDesc myDesc;

			myDesc.vertexShader = L"resources/shaders/shadedCube.hlsl";
			myDesc.pixelShader = L"resources/shaders/shadedCube.hlsl";

			myDesc.vertexLayout.push_back(vertexType::Position3);
			myDesc.vertexLayout.push_back(vertexType::Normal3);
			myDesc.vertexLayout.push_back(vertexType::TexCoord2);

			TextureDesc myTextureDesc{};
			myTextureDesc.renderTargetName = "shadowMapBuffer";
			myTextureDesc.useDepthBuffer = true;

			myDesc.textures.push_back(myTextureDesc);

			myDesc.samplers.push_back({ "sampler" });

			buffer = new MVPBuffer();

			myDesc.constBuffers.push_back({ "mvp", sizeof(MVPBuffer), buffer });

			drawable->setPipeline(myDesc);
		}

		{
			MeshDesc myDesc;

			myDesc = ResourceLoader::getMesh("resources/meshes/TheCube.obj");

			drawable->setMesh(myDesc);
		}
	}

	// setup for shadow pass
	{
		shadowPassDrawable = new Drawable();

		{
			PipelineObjectDesc myDesc;

			myDesc.vertexShader = L"resources/shaders/cubeShadowMap.hlsl";
			myDesc.pixelShader = L"resources/shaders/cubeShadowMap.hlsl";

			myDesc.vertexLayout.push_back(vertexType::Position3);
			myDesc.vertexLayout.push_back(vertexType::Normal3);
			myDesc.vertexLayout.push_back(vertexType::TexCoord2);

			myDesc.cullFront = true;

			shadowPassBuffer = new ShadowBuffer();

			myDesc.constBuffers.push_back({ "mvp", sizeof(ShadowBuffer), shadowPassBuffer });

			shadowPassDrawable->setPipeline(myDesc);
		}

		{
			MeshDesc myDesc;

			myDesc = ResourceLoader::getMesh("resources/meshes/TheCube.obj");

			shadowPassDrawable->setMesh(myDesc);
		}
	}
}

void TheCube::update(float aDeltaTime)
{
	//lightPos += aDeltaTime * 1;

	//if (lightPos > 5) lightPos = -5;

	glm::mat4 biasMatrix(
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0
	);

	glm::vec3 lightPosition{ 10, 10, -10 };
	glm::vec3 lightDirection{ -1, -1, 1 };
	lightDirection = glm::normalize(lightDirection);

	glm::mat4 lightViewMat = glm::lookAtLH(lightPosition, lightPosition + lightDirection, glm::vec3(0, 1, 0));

	const float aspectRatio = 1.f;
	const float horizontalFov = 3.141592f / 2.f;

	//glm::mat4 lightProjectionMat2 = glm::orthoLH_ZO(-10.f, 10.f, -10.f, 10.f, 0.1f, 1000.f);

	glm::mat4 lightProjectionMat = glm::perspectiveLH_ZO(horizontalFov, aspectRatio, 0.1f, 150.f);

	shadowPassBuffer->modelMatrix = getModelMatrix();
	shadowPassBuffer->viewProjectionMatrix = lightProjectionMat * lightViewMat;

	

	buffer->depthBiasMVP = biasMatrix * shadowPassBuffer->viewProjectionMatrix;

	buffer->color = glm::vec4(color, 1.f);
	buffer->modelMatrix = getModelMatrix();
	buffer->viewProjectionMatrix = camera->getProjectionMatrix() * camera->getViewMatrix();
}

void TheCube::shutdown()
{
	delete drawable;
	delete buffer;

	delete shadowPassDrawable;
	delete shadowPassBuffer;
}

Job* TheCube::getJob()
{
	Job* myJob = new Job(drawable, "Cube");

	myJob->addBufferUpdate({ "mvp", buffer, sizeof(MVPBuffer) });

	return myJob;
}

Job* TheCube::getShadowPassJob()
{
	Job* myJob = new Job(shadowPassDrawable, "Cube");

	myJob->addBufferUpdate({ "mvp", shadowPassBuffer, sizeof(ShadowBuffer) });

	return myJob;
}

glm::mat4 TheCube::getModelMatrix()
{
	glm::mat4 myTranslateMatrix = glm::translate(glm::mat4(1), position);
	glm::mat4 myScaleMatrix = glm::scale(glm::mat4(1), scale);

	glm::mat4 myRotateX = glm::rotate(glm::mat4(1), rotation.x, glm::vec3(1, 0, 0));
	glm::mat4 myRotateY = glm::rotate(glm::mat4(1), rotation.y, glm::vec3(0, 1, 0));
	glm::mat4 myRotateZ = glm::rotate(glm::mat4(1), rotation.z, glm::vec3(0, 0, 1));

	return myTranslateMatrix * myRotateZ * myRotateY * myRotateX * myScaleMatrix;
}
