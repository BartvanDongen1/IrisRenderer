#include "engine/resourceLoader.h"
#include "rendering/resources/resourceDescs.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobj/tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <unordered_map>

MeshDesc loadMesh(const char* aPath);
TextureDesc loadTexture(const char* aPath);

std::string GetFilePathExtension(const std::string& FileName);

std::unordered_map<const char*, MeshDesc> meshMap;
std::unordered_map<const char*, TextureDesc> textureMap;


MeshDesc ResourceLoader::getMesh(const char* aPath)
{
	auto searchItterator = meshMap.find(aPath);
	if (searchItterator == meshMap.cend())
	{
		// mesh isnt loaded yet

		MeshDesc myMeshDesc = loadMesh(aPath);
		meshMap.insert({ aPath, myMeshDesc });

		return myMeshDesc;
	}

	// mesh is loaded

	return meshMap[aPath];
}

TextureDesc ResourceLoader::getTexture(const char* aPath)
{
	auto searchItterator = textureMap.find(aPath);
	if (searchItterator == textureMap.cend())
	{
		// texture isnt loaded yet

		TextureDesc myTextureDesc = loadTexture(aPath);
		textureMap.insert({ aPath, myTextureDesc });

		return myTextureDesc;
	}

	// texture is loaded

	return textureMap[aPath];
}

MeshDesc loadMesh(const char* aPath)
{
	// load from .obj
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warning;
	std::string error;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, aPath);

	// Temp Buffer.
	std::vector<float> rawData;
	int indexOffset = 0;
	for (int f = 0; f < shapes[0].mesh.num_face_vertices.size(); f++)
	{
		int fv = shapes[0].mesh.num_face_vertices[f];
		for (int v = 0; v < fv; v++)
		{
			tinyobj::index_t idx = shapes[0].mesh.indices[indexOffset + v];
			// Vertex, Normal and UV coordinate (8)
			rawData.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
			rawData.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
			rawData.push_back(attrib.vertices[3 * idx.vertex_index + 2]);
			rawData.push_back(attrib.normals[3 * idx.normal_index + 0]);
			rawData.push_back(attrib.normals[3 * idx.normal_index + 1]);
			rawData.push_back(attrib.normals[3 * idx.normal_index + 2]);
			rawData.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
			rawData.push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
		}
		indexOffset += fv;
	}

	size_t totalSize = rawData.size();
	size_t vertexCount = totalSize / 8;

	// make mesh
	MeshDesc myMeshDesc;

	myMeshDesc.dataSize = totalSize * sizeof(float);
	myMeshDesc.data = malloc(myMeshDesc.dataSize);
	memcpy(myMeshDesc.data, rawData.data(), myMeshDesc.dataSize);

	myMeshDesc.vertexLayout.push_back(vertexType::Position3);
	myMeshDesc.vertexLayout.push_back(vertexType::Normal3);
	myMeshDesc.vertexLayout.push_back(vertexType::TexCoord2);

	return myMeshDesc;
}

TextureDesc loadTexture(const char* aPath)
{
	// load image
	stbi_set_flip_vertically_on_load(true);
	int width;
	int height;
	int comp;
	unsigned char* data = nullptr;
	data = stbi_load(aPath, &width, &height, &comp, 4);

	// assert data is loaded
	assert(data);

	int totalMemSize = sizeof(unsigned char) * comp * width * height;

	// make texture
	TextureDesc myTextureDesc;

	myTextureDesc.width = width;
	myTextureDesc.height = height;
	myTextureDesc.bytesPerPixel = comp;
	myTextureDesc.data = data;

	return myTextureDesc;
}

std::string GetFilePathExtension(const std::string& FileName)
{
	if (FileName.find_last_of(".") != std::string::npos)
		return FileName.substr(FileName.find_last_of(".") + 1);
	return "";
}
