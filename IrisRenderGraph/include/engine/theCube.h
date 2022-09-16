#pragma once
#include <glm/matrix.hpp>

class Drawable;
class Camera;
class Job;

struct MVPBuffer
{
	glm::mat4 viewProjectionMatrix;
	glm::mat4 modelMatrix;

	glm::vec4 viewPosition;

	glm::vec4 color;

	glm::mat4 depthBiasMVP;
	glm::vec4 lightPosition;

	int properties;

	float padding[3];
};

struct ShadowBuffer
{
	glm::mat4 viewProjectionMatrix;
	glm::mat4 modelMatrix;

	float padding[32];
};

class TheCube
{
public:
	TheCube() {};

	void init(const char* aModel = "resources/meshes/TheCube.obj");
	void update(float aDeltaTime);
	void shutdown();

	Job* getJob();
	Job* getShadowPassJob();

	void setSpecular(bool value = true);
	void setCamera(Camera* aCamera) { camera = aCamera; };
	glm::mat4 getModelMatrix();

	glm::vec3 position{ 0, 0, 0 };
	glm::vec3 rotation{ 0, 0, 0 };
	glm::vec3 scale{ 1, 1, 1 };

	glm::vec3 color{ 0, 0, 0 };

	float lightPos{ 0 };
private:

	Camera* camera;

	Drawable* drawable;
	Drawable* shadowPassDrawable;
	
	MVPBuffer* buffer;
	ShadowBuffer* shadowPassBuffer;
};

