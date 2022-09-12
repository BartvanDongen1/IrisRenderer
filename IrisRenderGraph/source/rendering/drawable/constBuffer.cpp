#include "rendering/drawable/constBuffer.h"
#include <cassert>

void ConstBuffer::update(void* aData, size_t aSize)
{
	assert(constantBufferSize == aSize);

	memcpy(pCbvDataBegin, aData, aSize);
}
