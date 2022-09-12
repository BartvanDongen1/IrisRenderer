#pragma once
#include "rendering/resources/resourceDescs.h"

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

