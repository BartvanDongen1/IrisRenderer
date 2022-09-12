#include "engine\engine.h"

int main()
{
	Engine myEngine;

	myEngine.init();

	Timer timer;

	while (!myEngine.shouldClose())
	{
		float deltaTime = static_cast<float>(timer.getFrameTime());

		myEngine.update(deltaTime);
	}

	myEngine.shutdown();

	return 0;
}