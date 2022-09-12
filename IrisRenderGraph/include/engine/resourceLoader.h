#pragma once

struct MeshDesc;
struct TextureDesc;

class ResourceLoader
{
public:
	static MeshDesc getMesh(const char* aPath);
	static TextureDesc getTexture(const char* aPath);

};


