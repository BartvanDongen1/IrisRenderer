#pragma once
#include <glm/glm.hpp>

class Camera
{
	friend class Controller;
public:
	Camera() {};
	~Camera() {};

	void init(glm::vec3 aPosition = glm::vec3(0,0,0), glm::vec3 aLookAtDirection = glm::vec3(0, 0, 1));

	glm::mat4 getViewMatrix();
	glm::mat4 getProjectionMatrix();

	glm::vec3 getPosition();

private:
	glm::vec3 position{ 0,0,0 };
	glm::vec3 lookAtDirection{ 0,0,1 };

	float nearPlane{ 0.1f };
	float farPlane{ 8000.f };
};
