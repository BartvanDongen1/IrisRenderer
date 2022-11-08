#include "rendering/renderGraph/job.h"
#include "rendering/drawable/drawable.h"
#include "rendering/drawable/bufferUpdate.h"
#include "rendering/drawable/descriptor.h"
#include "rendering/renderBackend.h"
#include "rendering\renderGraph\renderPassGraph.h"
#include "engine\camera.h"

Job::Job(Drawable2* aDrawable, const char* aName)
	: drawable2(aDrawable), name(aName) , usingDrawable2(true)
{
	
}

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

void Job::addstandardBufferUpdate(BufferUpdate aBufferUpdate)
{
	standardBufferUpdates.push_back(aBufferUpdate);
}

void Job::execute()
{
	if (!usingDrawable2)
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
	else
	{
		//for(int i = 0; i < drawable2->)

		for (int i = 0; i < standardBufferUpdates.size(); i++)
		{
			BufferUpdate update = standardBufferUpdates[i];

			if (update.bufferName == "transform")
			{
				glm::mat4 myTopLevelTransform = *(reinterpret_cast<glm::mat4*>(update.data));

				for (int i = 0; i < drawable2->primitives.size(); i++)
				{
					ModelNode* myNode = drawable2->primitiveNodes[i];
					ModelUniformBufffer* myBuffer = drawable2->uniformBuffers[i];

					glm::mat4 myWorldTransform = myTopLevelTransform * getLocalTransform(myNode);

					myBuffer->modelMatrix = myWorldTransform;
				}
			}
		}

		//update camera
		{
			auto resource = parent->getBufferResource("camera");

			Camera* myCamera = reinterpret_cast<Camera*>(resource->data);

			for (int i = 0; i < drawable2->primitives.size(); i++)
			{
				ModelUniformBufffer* myBuffer = drawable2->uniformBuffers[i];
				myBuffer->viewPosition = glm::vec4(myCamera->getPosition(), 1);
				myBuffer->viewProjectionMatrix = myCamera->getProjectionMatrix() * myCamera->getViewMatrix();
			}
		}

		//update standard uniform buffer
		for (int i = 0; i < drawable2->primitives.size(); i++)
		{
			ModelPrimitive* myPrimitive = drawable2->primitives[i];
			ModelUniformBufffer* myBuffer = drawable2->uniformBuffers[i];

			Descriptor* myDescriptor = myPrimitive->pipelineObject->descriptors.find("mvp")->second;

			myDescriptor->update(myBuffer, sizeof(ModelUniformBufffer));
		}



		// draw the model
		RenderBackend::drawDrawable2(drawable2);
	}
}
