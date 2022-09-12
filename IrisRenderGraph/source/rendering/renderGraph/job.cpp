#include "rendering/renderGraph/job.h"
#include "rendering/drawable/drawable.h"
#include "rendering/drawable/bufferUpdate.h"
#include "rendering/drawable/descriptor.h"
#include "rendering/renderBackend.h"
#include "rendering\renderGraph\renderPassGraph.h"

void Job::addPass(const char* aPass)
{
	passes.insert(aPass);
}

bool Job::containsPass(const char* aPass)
{
	return passes.contains(aPass);
}

void Job::addBufferUpdate(BufferUpdate aBufferUpdate)
{
	bufferUpdates.push_back(aBufferUpdate);
}

void Job::execute()
{
	// update all buffers
	for (int i = 0; i < bufferUpdates.size(); i++)
	{
		BufferUpdate update = bufferUpdates[i];

		if (update.graphResourceName != "")
		{
			auto resource = parent->getBufferResource(update.graphResourceName);

			drawable->getDescriptor(update.bufferName)->update(resource->data, resource->size);
		}
		else
		{
			drawable->getDescriptor(update.bufferName)->update(update.data, update.dataSize);
		}
	}

	// draw the model
	RenderBackend::drawDrawable(drawable);
}
