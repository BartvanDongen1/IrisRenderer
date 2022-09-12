#include "rendering/renderer.h"
#include "rendering/renderBackend.h"
#include "rendering/renderGraph/job.h"

#include "rendering/drawable/drawable.h"
#include "rendering/drawable/bufferUpdate.h"

#include "rendering/renderGraph/renderPassGraph.h"
#include "rendering/renderGraph/passes/clearPass.h"
#include "rendering/renderGraph/passes/drawPass.h"
#include "rendering/renderGraph/passes/fullScreenPass.h"
#include "rendering/debugModelBuilder.h"

#include "engine/resourceLoader.h"
#include "engine/theCube.h"
#include "engine/skybox.h"
#include "engine/camera.h"

static RenderBackend backend;
static RenderPassGraph* graph;

static int framesThisSecond{ 0 };
static float frameTimeAccumilator{ 0.f };

static int fps{ 0 };

static TheCube* cube1 = new TheCube();
static TheCube* cube2 = new TheCube();
static TheCube* cube3 = new TheCube();

static Skybox* skybox = new Skybox();

static Camera* camera{ nullptr };

bool Renderer::init()
{
	backend.loadPipeline();
	backend.loadAssets();

	backend.initImGui();

	graph = new RenderPassGraph();

	RenderBackend::setGraphForResources(graph);

	// setup skybox
	{
		skybox->init();

		Job* myJob = skybox->getJob();
		myJob->addPass("draw");

		graph->addJob(myJob);
	}

	{
		cube1->init();

		cube1->position = glm::vec3(0, -2, 0);
		
		cube1->scale.x = 100.0f;
		cube1->scale.y = 1.0f;
		cube1->scale.z = 100.0f;

		cube1->color = glm::vec3(0.1, 0.8, 0.1);

		Job* myJob = cube1->getJob();
		myJob->addPass("draw");

		graph->addJob(myJob);

		Job* myShadowPassJob = cube1->getShadowPassJob();
		myShadowPassJob->addPass("shadow");

		graph->addJob(myShadowPassJob);
	}

	{
		cube2->init();

		cube2->position = glm::vec3(2, 0, 0);

		cube2->scale.x = 1.0f;
		cube2->scale.y = 1.0f;
		cube2->scale.z = 1.0f;

		cube2->color = glm::vec3(0.1, 0.1, 0.8);
		//cube2->color = glm::vec3(1, 1, 1);

		Job* myJob = cube2->getJob();
		myJob->addPass("draw");

		graph->addJob(myJob);

		Job* myShadowPassJob = cube2->getShadowPassJob();
		myShadowPassJob->addPass("shadow");

		graph->addJob(myShadowPassJob);
	}

	{
		cube3->init();

		cube3->position = glm::vec3(-2, 0, 0);

		cube3->scale.x = 1.0f;
		cube3->scale.y = 1.0f;
		cube3->scale.z = 1.0f;

		cube3->color = glm::vec3(0.8, 0.1, 0.1);
		//cube3->color = glm::vec3(1, 1, 1);

		Job* myJob = cube3->getJob();
		myJob->addPass("draw");

		//graph->addJob(myJob);

		Job* myShadowPassJob = cube3->getShadowPassJob();
		myShadowPassJob->addPass("shadow");

		//graph->addJob(myShadowPassJob);
	}


	//create debug lines
	{
		glm::vec3 lightPosition{ 10, 10, -10 };
		glm::vec3 lightDirection{ -1, -1, 1 };

		DebugModelBuilder::drawSphere(glm::vec4(lightPosition, 1), 1, glm::vec4(1,1,1,1), 16);

		DebugModelBuilder::drawSphere(glm::vec4(lightPosition + lightDirection / 2.f, 1), 0.5, glm::vec4(1, 0, 0, 1), 16);
		DebugModelBuilder::drawSphere(glm::vec4(lightPosition + lightDirection, 1), 0.5, glm::vec4(1, 0, 0, 1), 16);

		/*for (int i = 0; i < 200; i++)
		{
			DebugModelBuilder::drawLine(glm::vec4(i / 2.f, 0, 0, 1), glm::vec4(i / 2.f, 1, 0, 1), glm::vec4(1, 0, 0, 1));
		}*/
	}

	graph->buildAndValidate();

	return true;
}

void Renderer::shutdown()
{
	backend.shutdown();
}

void Renderer::update(float aDeltaTime)
{
	//update skybox
	skybox->update();
	
	// monkey stuff
	cube1->update(aDeltaTime);
	cube2->update(aDeltaTime);
	cube3->update(aDeltaTime);

	frameTimeAccumilator += aDeltaTime;
	framesThisSecond++;

	if (frameTimeAccumilator > 0.2f)
	{
		frameTimeAccumilator -= 0.2f;
		fps = framesThisSecond * 5;
		framesThisSecond = 0;
	}

	backend.beginFrame();

	//do frame stuff
	graph->execute();

	backend.renderDebugModel(camera);

	backend.renderImGui(fps);

	backend.endFrame();
}

void Renderer::setCamera(Camera* aCamera)
{
	camera = aCamera;

	// temp set camera to the model, but should do it through the render graph
	cube1->setCamera(aCamera);
	cube2->setCamera(aCamera);
	cube3->setCamera(aCamera);
	skybox->setCamera(aCamera);
}
