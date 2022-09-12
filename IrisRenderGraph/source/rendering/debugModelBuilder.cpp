#include "rendering/debugModelBuilder.h"

#define PI 3.1415926535f

int currentVertexCount{ 0 };
PointVertex vertices[MAX_VERTICES];

void DebugModelBuilder::drawLine(PointVertex aPoint1, PointVertex aPoint2)
{
	assert(currentVertexCount + 1 < MAX_VERTICES);

	vertices[currentVertexCount++] = aPoint1;
	vertices[currentVertexCount++] = aPoint2;
}

void DebugModelBuilder::drawLine(glm::vec4 aPosition1, glm::vec4 aPosition2, glm::vec4 aColor)
{
	drawLine(PointVertex(aPosition1, aColor), PointVertex(aPosition2, aColor));
}

float map(float aValue, float aMin, float aMax, float aMappedMin, float aMappedMax)
{
	float x = (aValue - aMin) / (aMax - aMin);

	return x * (aMappedMax - aMappedMin) + aMappedMin;
}

void DebugModelBuilder::drawSphere(glm::vec4 aPosition, float aRadius, glm::vec4 aColor, int aResolution)
{
	int myResolutionX = aResolution * 2;
	int myResolutionY = aResolution;

	// create a temp 2d array for the points
	glm::vec4** myTempPositions;

	myTempPositions = new glm::vec4*[myResolutionY + 1];

	//generate points on the sphere  as if it were mapped to a plane 
	for (int i = 0; i < myResolutionY + 1; i++)
	{
		myTempPositions[i] = new glm::vec4[myResolutionX + 1];

		float latitude = map(i, 0, myResolutionY, 0, PI);

		for (int j = 0; j < myResolutionX + 1; j++)
		{
			float longitude = map(j, 0, myResolutionX, 0, PI * 2);

			glm::vec4 myTempPosition(0);

			myTempPosition.x = aRadius * sinf(longitude) * cosf(latitude);

			//y and z are flipped to make the polar points at the top and bottom
			myTempPosition.z = aRadius * sinf(longitude) * sinf(latitude);
			myTempPosition.y = aRadius * cosf(longitude);

			myTempPositions[i][j] = myTempPosition + aPosition;
		}
	}

	//draw lines based on the points
	for (int i = 0; i < myResolutionY; i++)
	{
		for (int j = 0; j < myResolutionX; j++)
		{
			// vertical lines
			drawLine(myTempPositions[i][j], myTempPositions[i][j + 1], aColor);

			//horizontal lines
			drawLine(myTempPositions[i][j], myTempPositions[i + 1][j], aColor);

			//diagonal lines
			drawLine(myTempPositions[i][j], myTempPositions[i + 1][j + 1], aColor);
		}
	}
}

void* DebugModelBuilder::getVertexData()
{
	return vertices;
}

int DebugModelBuilder::getVertexCount()
{
	return currentVertexCount;
}
