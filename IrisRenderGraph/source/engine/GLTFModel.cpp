#include "engine/GLTFModel.h"
#include "engine/resourceLoader.h"
#include "rendering/drawable/drawable.h"
#include "rendering/renderGraph/job.h"
#include "engine/modelMatrix.h"

GLTFModel::GLTFModel()
{
	modelMat = new ModelMatrix();
	modelMat->init();
}

void GLTFModel::initModel(const char* aModel)
{
	drawable = new Drawable2();
	Model* myModel = ResourceLoader::getGltfModel(aModel);

	// setup for draw pass
	{
		drawable->init(myModel);
	}

	// setup for shadow pass
	{
		//todo: implement
	}
}

void GLTFModel::updateModel()
{
	modelMat->setScale({ 0.05,0.05,0.05 });
	modelMat->setPosition({ -10, -1, 0 });

	modelMatrix = modelMat->getModelMatrix();
}

Job* GLTFModel::getJob()
{
	Job* myJob = new Job(drawable, "Cube");

	myJob->addstandardBufferUpdate({ .bufferName = "transform", .data = &modelMatrix, .dataSize = sizeof(glm::mat4)});

	return myJob;
}
