#pragma once
#include "basePass.h"

class Job;

class FullScreenPass : public BasePass
{
public:
	FullScreenPass(const char* aName);
	~FullScreenPass() override;

	void init() override;
	void execute() override;

private:
	Job* fullscreenJob;
};

