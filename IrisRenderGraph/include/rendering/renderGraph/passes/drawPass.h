#pragma once
#include "basePass.h"

class DrawPass : public BasePass
{
public:
	DrawPass(const char* aName);
	~DrawPass() {};

	void execute() override;

private:

};

