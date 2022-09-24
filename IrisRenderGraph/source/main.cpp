#include "engine\engine.h"

int main()
{
	Engine myEngine;
	Timer timer;

	myEngine.init();

	while (!myEngine.shouldClose())
	{
		float deltaTime = static_cast<float>(timer.getFrameTime());

		myEngine.update(deltaTime);
	}

	myEngine.shutdown();

	return 0;
}