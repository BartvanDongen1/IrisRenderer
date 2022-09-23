#pragma once

struct MeshDesc;
struct TextureDesc;
struct Model;

class ResourceLoader
{
public:
	static MeshDesc getMesh(const char* aPath);
	static TextureDesc getTexture(const char* aPath);
	static Model* getGltfModel(const char* aPath);
};


