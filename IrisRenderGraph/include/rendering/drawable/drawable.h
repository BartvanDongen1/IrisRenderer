#pragma once
#include "rendering/resources/resourceDescs.h"

#include "model.h"

class PipelineObject;
class Mesh;
class Descriptor;

class Drawable
{
	friend class RenderBackend;
public:
	Drawable() {};

	void initAsDefaultTriangle();
	void bind();

	void setPipeline(PipelineObjectDesc aDesc);
	void setMesh(MeshDesc aDesc);

	Descriptor* getDescriptor(const char* aName);

private:
	PipelineObject* pipeline{ nullptr };
	Mesh* mesh{ nullptr };

};

class Drawable2
{
	friend class RenderBackend;
	friend class Job;
public:
	Drawable2() {};

	void init(Model* aModel);

	Descriptor* getDescriptor(const char* aName, int aPrimitiveIndex);

private:
	void checkNodeForPrimitives(ModelNode* aNode);

	std::vector<ModelPrimitive*> primitives;
	std::vector<ModelNode*> primitiveNodes;
	std::vector<ModelUniformBufffer*> uniformBuffers;

	glm::mat4 transform;
};