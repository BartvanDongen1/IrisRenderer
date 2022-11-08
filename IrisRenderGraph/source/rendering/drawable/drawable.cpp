#include "engine\logger.h"
#include "rendering/drawable/drawable.h"
#include "rendering/renderBackend.h"
#include "rendering/drawable/pipelineObject.h"
#include "rendering/drawable/descriptor.h"
#include "engine/resourceLoader.h"
#include <glm/matrix.hpp>

struct offsetBuffer
{
	

	float offset[4]{ 0 };

	float padding[15 * 4];
};

void Drawable::initAsDefaultTriangle()
{
	{
		PipelineObjectDesc myDesc;

		myDesc.vertexShader = L"resources/shaders/texturedOffsetTriangle.hlsl";
		myDesc.pixelShader = L"resources/shaders/texturedOffsetTriangle.hlsl";

		myDesc.vertexLayout.push_back(vertexType::Position3);
		myDesc.vertexLayout.push_back(vertexType::Normal3);
		myDesc.vertexLayout.push_back(vertexType::TexCoord2);

		myDesc.textures.push_back(ResourceLoader::getTexture("resources/textures/monkey.png"));

		myDesc.samplers.push_back({ "sampler" });

		offsetBuffer* buffer = new offsetBuffer();

		buffer->offset[0] = 0.0f;
		buffer->offset[1] = 0.0f;

		myDesc.constBuffers.push_back({ "offset", sizeof(offsetBuffer), buffer});

		setPipeline(myDesc);

		pipeline->descriptors.find("offset")->second->update(buffer, sizeof(offsetBuffer));
	}

	{
		MeshDesc myDesc;
	
		// position3, normal3, texcoord2
		float triangleVerices[] =
		{
			 0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
			 0.4f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
			-0.4f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f
		};
	
		myDesc.vertexData = triangleVerices;
		myDesc.vertexDataSize = sizeof(triangleVerices);
	
		myDesc.vertexLayout.push_back(vertexType::Position3);
		myDesc.vertexLayout.push_back(vertexType::Normal3);
		myDesc.vertexLayout.push_back(vertexType::TexCoord2);
	
		setMesh(myDesc);
	}
}

void Drawable::bind()
{
}

void Drawable::setPipeline(PipelineObjectDesc aDesc)
{
	pipeline = RenderBackend::createPipelineObject(aDesc);
}

void Drawable::setMesh(MeshDesc aDesc)
{
	mesh = RenderBackend::createMesh(aDesc);
}

Descriptor* Drawable::getDescriptor(const char* aName)
{
	Descriptor* myDescriptor = pipeline->descriptors.find(aName)->second;

	return myDescriptor;
}






void Drawable2::init(Model* aModel)
{
	// get all primitives
	for (int i = 0; i < aModel->nodeCount; i++)
	{
		checkNodeForPrimitives(aModel->nodes[i]);
	}

	// set pipeline descs
	for (auto& myPrimitive : primitives)
	{
		if (myPrimitive->initialized)
		{
			LOG_WARNING("primitive already initialized");
			continue;
		}

		myPrimitive->pipelineObjectDesc.vertexLayout = myPrimitive->meshDesc.vertexLayout;
		myPrimitive->pipelineObjectDesc.samplers.push_back({});

		// standard const buffers

		uniformBuffers.push_back(new ModelUniformBufffer());
		myPrimitive->pipelineObjectDesc.constBuffers.push_back({ .name = "mvp", .size = sizeof(ModelUniformBufffer) });
		//todo: pushback some lighting const buffer

		myPrimitive->pipelineObjectDesc.vertexShader = L"resources/shaders/shadedModel.hlsl";
		myPrimitive->pipelineObjectDesc.pixelShader = L"resources/shaders/shadedModel.hlsl";

		// textures for the shader
		myPrimitive->pipelineObjectDesc.textures.push_back(myPrimitive->material.textureBaseColor);
		myPrimitive->pipelineObjectDesc.textures.push_back(myPrimitive->material.textureMetallicRoughness);
		myPrimitive->pipelineObjectDesc.textures.push_back(myPrimitive->material.textureNormal);
	}

	// init all meshes and pipeline descs
	for (auto& myPrimitive : primitives)
	{
		myPrimitive->mesh = RenderBackend::createMesh(myPrimitive->meshDesc);
		myPrimitive->pipelineObject = RenderBackend::createPipelineObject(myPrimitive->pipelineObjectDesc);
	}
}

Descriptor* Drawable2::getDescriptor(const char* aName, int aPrimitiveIndex)
{
	Descriptor* myDescriptor = primitives[aPrimitiveIndex]->pipelineObject->descriptors.find(aName)->second;

	return myDescriptor;
}

void Drawable2::checkNodeForPrimitives(ModelNode* aNode)
{
	if (aNode->piece)
	{
		ModelPiece* myPiece = aNode->piece;

		primitives.push_back(myPiece->primitives);
		primitiveNodes.push_back(aNode);
	}

	for (int i = 0; i < aNode->childCount; i++)
	{
		checkNodeForPrimitives(aNode->children[i]);
	}
}
