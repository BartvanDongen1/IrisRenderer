#include "rendering/renderGraph/passes/clearPass.h"
#include "rendering\renderTarget.h"
#include "rendering\renderBackend.h"

ClearPass::ClearPass(const char* aName)
	:
	BasePass(aName)
{
	addWriteResource("buffer");
}

ClearPass::~ClearPass()
{
}

void ClearPass::execute()
{
	RenderBackend::beginScene(resources.find("buffer")->second.first);

	RenderBackend::clearRenderTarget(resources.find("buffer")->second.first);

	RenderBackend::beginScene(resources.find("buffer")->second.first);
}
