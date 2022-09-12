#pragma once
#include <vector>
#include <unordered_set>
#include "rendering/drawable/bufferUpdate.h"

class Drawable;
class RenderPassGraph;

class Job
{
	friend class RenderPassGraph;
public:
	Job(Drawable* aDrawable, const char* aName = "default") : drawable(aDrawable), name(aName) {};
	~Job() {};

	void addPass(const char* aPass);
	bool containsPass(const char* aPass);

	void addBufferUpdate(BufferUpdate aBufferUpdate);

	void execute();

private:
	RenderPassGraph* parent;

	std::unordered_set<const char*> passes;

	const char* name;
	Drawable* drawable;

	std::vector<BufferUpdate> bufferUpdates;
};

