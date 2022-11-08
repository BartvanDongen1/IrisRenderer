#pragma once
#include "mesh.h"
#include "pipelineObject.h"
#include "engine\aabb.h"

#include "rendering\resources\resourceDescs.h"

#include <glm/glm.hpp>

struct ModelUniformBufffer
{
	glm::mat4 viewProjectionMatrix;
	glm::mat4 modelMatrix;

	glm::vec4 viewPosition;

	glm::vec4 color{1, 1, 1, 1};

	glm::mat4 depthBiasMVP;
	glm::vec4 lightPosition;

	int properties;

	float padding[3];
};


struct Material
{
	bool unlit;

	glm::vec3 baseColorFactor;
	float metallicFactor;
	float roughnessFactor;
	TextureDesc textureBaseColor;
	TextureDesc textureMetallicRoughness;
	TextureDesc textureNormal;
};

struct ModelPrimitive
{
	union
	{
		PipelineObject* pipelineObject;
		PipelineObjectDesc pipelineObjectDesc;
	};

	union
	{
		Mesh* mesh;
		MeshDesc meshDesc;
	};

	bool initialized{ false };

	Material material;
};

struct ModelPiece
{
	char name[64];

	AABB aabb;

	size_t primitiveCount;
	ModelPrimitive* primitives;
};

struct ModelNode
{
	char name[64];

	ModelNode* parent;

	size_t childCount;
	ModelNode** children;

	glm::mat4 transform;

	ModelPiece* piece;
};

glm::mat4 getLocalTransform(ModelNode* aNode);

struct Model
{
	size_t nodeCount;
	ModelNode** nodes;

	AABB aabb;
};
