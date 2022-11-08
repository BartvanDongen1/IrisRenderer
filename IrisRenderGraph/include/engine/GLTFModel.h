#pragma once
#include <glm/matrix.hpp>

class Drawable2;
class Camera;
class Job;
class ModelMatrix;

class GLTFModel
{
public:
	GLTFModel();

	void initModel(const char* aModel);
	void updateModel();

	Job* getJob();

private:
	Drawable2* drawable;
	ModelMatrix* modelMat;

	glm::mat4 modelMatrix;
};

