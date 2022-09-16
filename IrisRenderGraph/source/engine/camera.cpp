#include "engine/camera.h"
#include "rendering/window.h"

#include <glm/gtc/matrix_transform.hpp>

constexpr auto PI = 3.1415926535;

void Camera::init(glm::vec3 aPosition, glm::vec3 aLookAtDirection) 
{
	position = aPosition;
	lookAtDirection = aLookAtDirection;
}

glm::mat4 Camera::getViewMatrix()
{
	return glm::lookAtLH(position, position + lookAtDirection, glm::vec3(0, 1, 0));
}

glm::mat4 Camera::getProjectionMatrix()
{
	const float aspectRatio = static_cast<float>(Window::getWidth()) / static_cast<float>(Window::getHeight());
	const float horizontalFov = PI / 2.0f;

	return glm::perspectiveLH_ZO(horizontalFov, aspectRatio, nearPlane, farPlane);
}

glm::vec3 Camera::getPosition()
{
	return position;
}
