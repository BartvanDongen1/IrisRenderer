#pragma once
#include <unordered_map>

class RenderPassGraph;
class RenderTarget;

class BasePass
{
public:
	BasePass() {};
	BasePass(const char* aName);
	virtual ~BasePass() {};

	virtual void init() {};
	virtual void execute();

	void bindResource(const char* aRegisteredName, const char* aTarget);

	RenderPassGraph* parent{ nullptr };
protected:

	void addReadResource(const char* aName);
	void addWriteResource(const char* aName);

	std::unordered_map < const char*, std::pair<RenderTarget*, const char*> > resources;

	const char* name{ "default" };
};