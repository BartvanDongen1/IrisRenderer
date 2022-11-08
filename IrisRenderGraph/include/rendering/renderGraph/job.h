#pragma once
#include <vector>
#include <unordered_set>
#include "rendering/drawable/bufferUpdate.h"

class Drawable;
class Drawable2;
class RenderPassGraph;

class Job
{
	friend class RenderPassGraph;
public:
	Job(Drawable* aDrawable, const char* aName = "default") : drawable(aDrawable), name(aName) {};
	Job(Drawable2* aDrawable, const char* aName = "default");
	~Job() {};

	void addPass(const char* aPass);
	bool containsPass(const char* aPass);

	void addBufferUpdate(BufferUpdate aBufferUpdate);
	void addstandardBufferUpdate(BufferUpdate aBufferUpdate);

	void execute();

private:
	RenderPassGraph* parent{ nullptr };

	std::unordered_set<const char*> passes;

	const char* name;

	bool usingDrawable2{ false };
	union
	{
		Drawable* drawable;
		Drawable2* drawable2;
	};

	std::vector<BufferUpdate> bufferUpdates;
	std::vector<BufferUpdate> standardBufferUpdates;
};