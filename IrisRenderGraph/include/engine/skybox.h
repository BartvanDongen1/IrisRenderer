#pragma once
#include <glm/glm.hpp>

class Drawable;
class Camera;
class Job;

struct SkyboxBuffer
{
	glm::mat4 viewProjectionMatrix;
	float padding[12 * 4];
};

class Skybox
{
public:
	Skybox() {};
	~Skybox() {};

	void init();
	void update();
	void shutdown();

	Job* getJob();

	void setCamera(Camera* aCamera) { camera = aCamera; };

private:
	Camera* camera{ nullptr };

	Drawable* drawable{ nullptr };
	SkyboxBuffer* skyboxBuffer{ nullptr };
};

