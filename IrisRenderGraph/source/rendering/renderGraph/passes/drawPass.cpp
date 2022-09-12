#include "rendering/renderGraph/passes/drawPass.h"
#include "rendering/renderGraph/renderPassGraph.h"
#include "rendering/renderGraph/job.h"
#include "rendering/renderBackend.h"

DrawPass::DrawPass(const char* aName)
	:
	BasePass(aName)
{}

void DrawPass::execute()
{
	RenderBackend::beginScene(resources.find("output")->second.first);

	for (int i = 0; i < parent->jobs.size(); i++)
	{
		if (parent->jobs[i]->containsPass(name))
		{
			parent->jobs[i]->execute();
		}

	}
	
	RenderBackend::endScene(resources.find("output")->second.first);
}
