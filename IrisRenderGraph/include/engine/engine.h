#pragma once
#include "rendering\window.h"
#include "rendering\renderer.h"
#include "engine\timer.h"
#include "engine\inputManager.h"
#include "engine\controller.h"

class Engine
{
public:
	void init();
	void update(float aDeltaTime);
	void shutdown();

	bool shouldClose();

private:
	Window window;
	Renderer renderer;

	InputManager inputManager;
	
	Controller controller;

	bool windowFocused{ false };
};

