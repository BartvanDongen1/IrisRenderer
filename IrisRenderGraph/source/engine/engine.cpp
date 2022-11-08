#include "engine/engine.h"
#include "engine/resourceLoader.h"
#include "engine\logger.h"

void Engine::init()
{
	window.init(1920, 1080, "Iris Application");

	controller.init();

	renderer.setCamera(controller.getCamera());
	renderer.init();

	inputManager.init();

	ResourceLoader::getGltfModel("resources/meshes/GLTF/fox/Fox.gltf");
}

void Engine::update(float aDeltaTime)
{
	window.processMessages();

	renderer.update(aDeltaTime);

	if (InputManager::getKey(Keys::f)->pressed)
	{
		if (windowFocused)
		{
			window.freeCursor();
			window.showCursor();
		}
		else
		{
			window.confineCursor();
			window.hideCursor();
		}

		windowFocused = !windowFocused;
	}

	if (windowFocused)
	{
		// only update controller if window in focus
		controller.update(aDeltaTime);
	}
}

void Engine::shutdown()
{
	renderer.shutdown();
	inputManager.shutdown();
}

bool Engine::shouldClose()
{
	return window.getShouldClose();
}
