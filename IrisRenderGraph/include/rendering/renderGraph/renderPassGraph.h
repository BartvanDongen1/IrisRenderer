#pragma once
#include "rendering\resources\resourceDescs.h"

#include <vector>
#include <unordered_map>

#include <wrl.h>
#include <d3d12.h>

class BasePass;
class RenderTarget;
class Job;

struct GraphBufferResource
{
	size_t size;
	void* data;
};

class RenderPassGraph
{
	friend class DrawPass;
	friend class RenderBackend;
public:
	RenderPassGraph();
	~RenderPassGraph() {};

	void addPass(BasePass* aPass);
	void addJob(Job* aJob);

	void addBufferResource(const char* aName, GraphBufferResource* aResource);

	bool buildAndValidate();
	
	void execute();

	GraphBufferResource* getBufferResource(const char* aName);
	RenderTarget* getResource(const char* aName);
private:
	bool isValidated{ false };
	std::vector<BasePass*> passes;
	std::vector<Job*> jobs;

	std::unordered_map<const char*, RenderTarget*> resourceMap;
	std::unordered_map<const char*, GraphBufferResource*> bufferResourceMap;
};
