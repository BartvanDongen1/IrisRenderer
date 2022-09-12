#include "rendering/renderGraph/passes/basePass.h"
#include "rendering/renderGraph/renderPassGraph.h"
#include "rendering/renderBackend.h"

#include "rendering\renderTarget.h"

BasePass::BasePass(const char* aName)
	:
	name(aName)
{}

void BasePass::execute()
{}

void BasePass::bindResource(const char* aRegisteredName, const char* aTarget)
{
	resources.insert({ aRegisteredName, {parent->getResource(aTarget), aTarget} });
}

void BasePass::addReadResource(const char* aName)
{
}

void BasePass::addWriteResource(const char* aName)
{
}
