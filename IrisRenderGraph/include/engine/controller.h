#pragma once
#include <glm/glm.hpp>
#include "camera.h"

class Controller
{
public:
	Controller();
	~Controller();

	void init();
	void update(float aDeltaTime);

	Camera* getCamera() { return camera; };

	float mouseSensitivityMultiplier{ 1 };

	float acceleration{ 20 };
	float deceleration{ 5 };

	float maxVelocity{ 4 };

private:
	Camera* camera;

	glm::vec3 position{ 0, 0, 0 };
	glm::vec3 velocity{ 0, 0, 0 };

	float pitch{ 0 };
	float yaw{ 0 };

	float speedScale{ 1 };
	float minSpeedScale{ 0.1 };
	float maxSpeedScale{ 5 };
};

