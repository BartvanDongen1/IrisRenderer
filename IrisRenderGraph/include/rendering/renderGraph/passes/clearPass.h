#pragma once
#include "basePass.h"

class ClearPass : public BasePass
{
public:
	ClearPass(const char* aName);
	~ClearPass();

	void execute() override;

private:

};

