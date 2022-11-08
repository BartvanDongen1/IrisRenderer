#pragma once
#include <glm/matrix.hpp>

class ModelMatrix
{
public:
	void init();
	
	void setPosition(glm::vec3 aPosition);
	void setScale(glm::vec3 aScale);
	void setRotation(glm::vec3 aRotation);

	glm::vec3 getPosition();
	glm::vec3 getScale();
	glm::vec3 getRotation();

	glm::mat4 getModelMatrix();

private:
	void updateMatrix();

	glm::mat4 modelMatrix;
	bool isDirty{ true };

	glm::vec3 position;
	glm::vec3 scale;
	glm::vec3 rotation;
};

