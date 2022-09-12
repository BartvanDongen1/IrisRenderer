#include "rendering/drawable/texture.h"
#include <cassert>

void Texture::update(void* aData, size_t aSize)
{
	size_t size = textureWidth * textureHeight * bytesPerPixel;

	assert(size == aSize);

	memcpy(textureData, aData, aSize);
}
