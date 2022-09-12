#include "rendering/renderGraph/renderPassGraph.h"
#include "rendering/renderGraph/passes/basePass.h"
#include "rendering\renderBackend.h"
#include "../helper.h"
#include "rendering/window.h"
#include "rendering\renderGraph\job.h"
#include "rendering/drawable/drawable.h"

#include "rendering/renderGraph/renderPassGraph.h"
#include "rendering/renderGraph/passes/clearPass.h"
#include "rendering/renderGraph/passes/drawPass.h"
#include "rendering/renderGraph/passes/fullScreenPass.h"

#include <cassert>

#include "rendering\renderTarget.h"
#include "rendering\descriptorHeap.h"
#include "rendering\resources\resourceDescs.h"

RenderPassGraph::RenderPassGraph()
{
	RenderTargetDesc myRenderTarget;
	myRenderTarget.size[0] = Window::getWidth(); 
	myRenderTarget.size[1] = Window::getHeight();

	RenderTarget* myBackBuffer = RenderBackend::createRenderTarget(myRenderTarget);
	resourceMap.insert( { "backBuffer", myBackBuffer } );

	RenderTarget* myPostInputBuffer = RenderBackend::createRenderTarget(myRenderTarget);
	resourceMap.insert({ "postInputBuffer", myPostInputBuffer });

	RenderTargetDesc myShadowMapDesc;
	myShadowMapDesc.size[0] = 720;
	myShadowMapDesc.size[1] = 720;

	RenderTarget* myShadowMapBuffer = RenderBackend::createRenderTarget(myShadowMapDesc);
	resourceMap.insert({ "shadowMapBuffer", myShadowMapBuffer });

	//add passes to the graph
	
	//setup clear pass
	{
		ClearPass* pass = new ClearPass("clear");
		addPass(pass);
		pass->bindResource("buffer", "backBuffer");
	}

	//setup clear pass
	{
		ClearPass* pass = new ClearPass("clear");
		addPass(pass);
		pass->bindResource("buffer", "postInputBuffer");
	}

	//setup clear pass
	{
		ClearPass* pass = new ClearPass("clear");
		addPass(pass);
		pass->bindResource("buffer", "shadowMapBuffer");
	}

	// setup shadow pass
	{
		DrawPass* pass = new DrawPass("shadow");
		addPass(pass);
		pass->bindResource("output", "shadowMapBuffer");
	}

	// setup draw pass
	{
		DrawPass* pass = new DrawPass("draw");
		addPass(pass);
		pass->bindResource("output", "postInputBuffer");
	}

	// setup post pass
	{
		FullScreenPass* pass = new FullScreenPass("post");
		addPass(pass);

		pass->bindResource("input", "postInputBuffer");
		pass->bindResource("output", "backBuffer");
	}
}

void RenderPassGraph::addPass(BasePass* aPass)
{
	isValidated = false;
	aPass->parent = this;
	passes.push_back(aPass);
}

void RenderPassGraph::addJob(Job* aJob)
{
	aJob->parent = this;
	jobs.push_back(aJob);
}

void RenderPassGraph::addBufferResource(const char* aName, GraphBufferResource* aResource)
{
	bufferResourceMap.insert({ aName, aResource });
}

bool RenderPassGraph::buildAndValidate()
{
	for (int i = 0; i < passes.size(); i++)
	{
		passes[i]->init();
	}

	isValidated = true;

	return true;
}

void RenderPassGraph::execute()
{
	assert(isValidated);

	for (int i = 0; i < passes.size(); i++)
	{
		passes[i]->execute();
	}

	RenderBackend::presentRenderTarget(getResource("backBuffer"));
}

GraphBufferResource* RenderPassGraph::getBufferResource(const char* aName)
{
	return bufferResourceMap.find(aName)->second;
}

RenderTarget* RenderPassGraph::getResource(const char* aName)
{
	return resourceMap.find(aName)->second;
}
