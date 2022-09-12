#pragma once

#include <glm/vec4.hpp>

#define MAX_VERTICES 1024 * 1024

struct PointVertex
{
	PointVertex() {}

	PointVertex(glm::vec4 aPosition, glm::vec4 aColor = glm::vec4(0, 1, 0, 1))
		: position(aPosition), color(aColor)
	{}

	glm::vec4 position = glm::vec4(0);
	glm::vec4 color = glm::vec4(0);
};

class DebugModelBuilder
{
	friend class RenderBackend;
public:
	DebugModelBuilder() {};
	~DebugModelBuilder() {};

	static void drawLine(PointVertex aPoint1, PointVertex aPoint2);
	static void drawLine(glm::vec4 aPosition1, glm::vec4 aPosition2, glm::vec4 aColor = glm::vec4(0,1,0,1));

	//static void drawCube();
	static void drawSphere(glm::vec4 aPosition, float aRadius, glm::vec4 aColor = glm::vec4(0, 1, 0, 1), int aResolution = 16);

private:
	static void* getVertexData();
	static int getVertexCount();
};

