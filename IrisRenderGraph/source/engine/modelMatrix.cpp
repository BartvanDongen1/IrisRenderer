#include "engine/modelMatrix.h"

#include <glm/gtc/matrix_transform.hpp>

void ModelMatrix::init()
{
	position = glm::vec3(0);
	scale = glm::vec3(1);
	rotation = glm::vec3(0);

	updateMatrix();

	isDirty = false;
}

void ModelMatrix::setPosition(glm::vec3 aPosition)
{
	position = aPosition;

	isDirty = true;
}

void ModelMatrix::setScale(glm::vec3 aScale)
{
	scale = aScale;

	isDirty = true;
}

void ModelMatrix::setRotation(glm::vec3 aRotation)
{
	rotation = aRotation;

	isDirty = true;
}

glm::vec3 ModelMatrix::getPosition()
{
	return position;
}

glm::vec3 ModelMatrix::getScale()
{
	return scale;
}

glm::vec3 ModelMatrix::getRotation()
{
	return rotation;
}

glm::mat4 ModelMatrix::getModelMatrix()
{
	if (isDirty) updateMatrix();
	isDirty = false;

	return modelMatrix;
}

void ModelMatrix::updateMatrix()
{
	glm::mat4 myTranslateMatrix = glm::translate(glm::mat4(1), position);
	glm::mat4 myScaleMatrix = glm::scale(glm::mat4(1), scale);

	glm::mat4 myRotateX = glm::rotate(glm::mat4(1), rotation.x, glm::vec3(1, 0, 0));
	glm::mat4 myRotateY = glm::rotate(glm::mat4(1), rotation.y, glm::vec3(0, 1, 0));
	glm::mat4 myRotateZ = glm::rotate(glm::mat4(1), rotation.z, glm::vec3(0, 0, 1));

	modelMatrix = myTranslateMatrix * myRotateZ * myRotateY * myRotateX * myScaleMatrix;
}
