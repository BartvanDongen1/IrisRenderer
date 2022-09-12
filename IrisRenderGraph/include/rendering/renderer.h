#pragma once
#include <Windows.h>

class Camera;

class Renderer
{
public:
	static bool init();
	static void shutdown();

	static void update(float aDeltaTime);

	static void setCamera(Camera* aCamera);
};

