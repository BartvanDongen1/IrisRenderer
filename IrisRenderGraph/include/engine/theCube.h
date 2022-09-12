#pragma once
#include <glm/matrix.hpp>

class Drawable;
class Camera;
class Job;

struct MVPBuffer
{
	glm::mat4 viewProjectionMatrix;
	glm::mat4 modelMatrix;

	glm::vec4 color;

	glm::mat4 depthBiasMVP;

	float padding[12];
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

	void init();
	void update(float aDeltaTime);
	void shutdown();

	Job* getJob();
	Job* getShadowPassJob();

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

