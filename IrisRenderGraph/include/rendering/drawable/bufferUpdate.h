#pragma once

struct BufferUpdate
{
	const char* bufferName;

	void* data;
	size_t dataSize;

	const char* graphResourceName{ "" };
};