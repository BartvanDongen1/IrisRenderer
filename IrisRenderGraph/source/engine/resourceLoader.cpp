#include "engine/resourceLoader.h"
#include "rendering/resources/resourceDescs.h"
#include "rendering/drawable/model.h"
#include "engine/utility.h"

#define CGLTF_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4996)
#include <cgltf/cgltf.h>
#pragma warning(pop)

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobj/tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#pragma warning(push, 0)
//#pragma warning(disable : 4068)
#include <stb/stb_image.h>
#pragma warning(pop)

#include <string.h>
#include <unordered_map>
#include  <glm/gtc/quaternion.hpp>
#include  <glm/gtx/transform.hpp>

MeshDesc loadMesh(const char* aPath);
TextureDesc loadTexture(const char* aPath);
Model* loadGltfModel(const char* aPath);

std::string GetFilePathExtension(const std::string& FileName);

std::unordered_map<const char*, MeshDesc> meshMap;
std::unordered_map<const char*, TextureDesc> textureMap;
std::unordered_map<const char*, Model*> modelMap;


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

Model* ResourceLoader::getGltfModel(const char* aPath)
{
	auto searchItterator = modelMap.find(aPath);
	if (searchItterator == modelMap.cend())
	{
		// texture isnt loaded yet

		Model* myModel = loadGltfModel(aPath);
		modelMap.insert({ aPath, myModel });

		return myModel;
	}

	// texture is loaded

	return modelMap[aPath];
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

	myMeshDesc.vertexDataSize = totalSize * sizeof(float);
	myMeshDesc.vertexData = malloc(myMeshDesc.vertexDataSize);
	memcpy(myMeshDesc.vertexData, rawData.data(), myMeshDesc.vertexDataSize);

	myMeshDesc.vertexLayout.push_back(vertexType::Position3);
	myMeshDesc.vertexLayout.push_back(vertexType::Normal3);
	myMeshDesc.vertexLayout.push_back(vertexType::TexCoord2);

	return myMeshDesc;
}

TextureDesc loadTexture(const char* aPath)
{
	// load image
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

// --------------------
//  GLTF file loading
// --------------------

// matrix to transform from GLTF coord space (X = left,  Y = up,  Z = forward)
// to my coord space						 (X = right, Y = up,  Z = forward)
static const glm::mat4 g_coordinateTransform{ -1, 0, 0, 0,
											   0, 1, 0, 0,
											   0, 0, 1, 0,
											   0, 0, 0, 1 };

template <typename T>
static T* GetBufferViewPointer(const cgltf_buffer_view* bufferView)
{
	char* base = static_cast<char*>(bufferView->buffer->data);
	base += bufferView->offset;

	return reinterpret_cast<T*>(base);
}

template <typename T>
static T* GetAccessorPointer(const cgltf_accessor* accessor)
{
	auto bufferView = accessor->buffer_view;

	char* base = static_cast<char*>(bufferView->buffer->data);
	base += bufferView->offset;
	base += accessor->offset;

	return reinterpret_cast<T*>(base);
}

static int PathSplitLocation(const char* path)
{
	int result = -1;

	const char* last = path;
	for (const char* c = path; *c; c += 1)
	{
		if (*c == '/' || *c == '\\')
		{
			result = static_cast<int> (c - path);
		}
	}

	return result;

}

#pragma warning(push)
#pragma warning(disable : 4996)

static int SplitPath(int destCount, char* dest, int sourceCount, const char* source)
{
	int at = PathSplitLocation(source);
	if (at != -1)
	{
		int copyCount = MIN(at + 1, sourceCount);
		copyCount = MIN(copyCount, destCount);
		strncpy(dest, source, copyCount);
		return copyCount;
	}
	return -1;
}

static void Append(int location, int destCount, char* dest, int sourceCount, char* source)
{
	assert(location <= destCount);
	int copyCount = MIN(destCount - location, sourceCount);
	assert(copyCount >= 0);
	strncpy(dest + location, source, (size_t)copyCount);
}

#pragma warning(pop)

static char* CreateTempPathFromUri(const char* base, const char* uri)
{
	static char path[4096]; // not very good, but okay. we don't have a temp allocator.
	assert(strlen(base) + strlen(uri) + 1 <= sizeof(path));

	cgltf_combine_paths(path, base, uri);
	cgltf_decode_uri(path + strlen(path) - strlen(uri));

	return path;
}

static __int64 GLTFMeshIndex(const cgltf_data* gltf, const cgltf_mesh* mesh)
{
	return mesh - gltf->meshes;
}

static __int64 GLTFImageIndex(const cgltf_data* gltf, const cgltf_image* image)
{
	return image - gltf->images;
}

static __int64 GLTFNodeIndex(const cgltf_data* gltf, const cgltf_node* node)
{
	return node - gltf->nodes;
}

static bool GLTFNodeHasMesh(const cgltf_node* node)
{
	if (node->mesh)
	{
		return true;
	}

	for (size_t i = 0; i < node->children_count; i += 1)
	{
		if (GLTFNodeHasMesh(node->children[i]))
		{
			return true;
		}
	}

	return false;
}

static glm::vec3 transformPoint(glm::mat4 transform, glm::vec3 p)
{
	return glm::vec3(transform * glm::vec4(p, 1));
}

static AABB TransformAABB(glm::mat4 transform, AABB aabb)
{
	aabb = growToContain(aabb, transformPoint(transform, glm::vec3(aabb.min.x, aabb.min.y, aabb.min.z)));
	aabb = growToContain(aabb, transformPoint(transform, glm::vec3(aabb.max.x, aabb.min.y, aabb.min.z)));
	aabb = growToContain(aabb, transformPoint(transform, glm::vec3(aabb.min.x, aabb.max.y, aabb.min.z)));
	aabb = growToContain(aabb, transformPoint(transform, glm::vec3(aabb.min.x, aabb.min.y, aabb.max.z)));
	aabb = growToContain(aabb, transformPoint(transform, glm::vec3(aabb.max.x, aabb.max.y, aabb.min.z)));
	aabb = growToContain(aabb, transformPoint(transform, glm::vec3(aabb.max.x, aabb.min.y, aabb.max.z)));
	aabb = growToContain(aabb, transformPoint(transform, glm::vec3(aabb.min.x, aabb.max.y, aabb.max.z)));
	aabb = growToContain(aabb, transformPoint(transform, glm::vec3(aabb.max.x, aabb.max.y, aabb.max.z)));
	return aabb;
}

static AABB CalculateAABB(ModelNode* node, AABB aabb = AABB::invertedInfinity(), glm::mat4 transform = glm::mat4(1))
{
	// only get bounds if node has them
	if (node->piece)
	{
		ModelPiece* piece = node->piece;
		AABB pieceBounds = TransformAABB(transform, piece->aabb);
		aabb = combine(pieceBounds, aabb);
	}

	transform = transform * node->transform;

	for (size_t i = 0; i < node->childCount; i += 1)
	{
		node->children[i];

		aabb = combine(aabb, CalculateAABB(node->children[i], aabb, transform));
	}

	return aabb;
}

//struct Vertex
//{
//	float position[3];
//	float normal[3] ;
//	float texcoord[2];
//};

Model* loadGltfModel(const char* aPath)
{
	Model* result = nullptr;

	cgltf_options options = {};
	cgltf_data* data = nullptr;

	if (cgltf_parse_file(&options, aPath, &data) == cgltf_result_success)
	{
		defer{ cgltf_free(data); };

		cgltf_load_buffers(&options, data, aPath);

		// find memory footprint of the full model
		size_t nodeCount = 0;
		size_t childNodeCount = 0;
		for (size_t i = 0; i < data->nodes_count; i += 1)
		{
			cgltf_node* node = &data->nodes[i];
			
			nodeCount += 1;
			childNodeCount += node->children_count;
		}

		cgltf_scene* scene = data->scene;
		if (scene)
		{
			childNodeCount += scene->nodes_count;
		}

		size_t pieceCount = data->meshes_count;

		size_t primitiveCount = 0;
		for (size_t i = 0; i < data->meshes_count; i += 1)
		{
			primitiveCount += data->meshes[i].primitives_count;
		}

		// allocate the memory
		void* memoryPool = malloc(sizeof(Model) +
			AlignAddress(sizeof(ModelNode) * nodeCount, 16) +
			AlignAddress(sizeof(ModelNode*) * childNodeCount, 16) +
			AlignAddress(sizeof(ModelPiece) * pieceCount, 16) +
			AlignAddress(sizeof(ModelPrimitive) * primitiveCount, 16));

		defer{ free(memoryPool); };

		// Dividing up the memory to all the different arrays we want
		Model* model = static_cast<Model*>(memoryPool);
		ModelNode* nodes = CastAlignPointer<ModelNode>(model + 1);
		ModelNode** childNodes = CastAlignPointer<ModelNode*>(nodes + nodeCount);
		ModelPiece* pieces = CastAlignPointer<ModelPiece>(childNodes + childNodeCount);
		ModelPrimitive* primitives = CastAlignPointer<ModelPrimitive>(pieces + pieceCount);

		zeroStruct(model);
		zeroArray(nodeCount, nodes);
		zeroArray(childNodeCount, childNodes);
		zeroArray(pieceCount, pieces);
		zeroArray(primitiveCount, primitives);

		// create texture descriptions
		auto images = new TextureDesc[data->textures_count]{};
		defer{ delete[] images; };

		for (size_t i = 0; i < data->textures_count; i += 1)
		{
			cgltf_texture* texture = &data->textures[i];
			cgltf_image* image = texture->image;

			int myWidth, myHeight, nComponents;
			unsigned char* myPixels = nullptr;

			if (image->buffer_view)
			{
				unsigned char* source = GetBufferViewPointer<unsigned char>(image->buffer_view);
				myPixels = stbi_load_from_memory(source, static_cast<int>(image->buffer_view->size), &myWidth, &myHeight, &nComponents, 4);
			}
			else
			{
				char* imagePath = CreateTempPathFromUri(aPath, image->uri);
				myPixels = stbi_load(imagePath, &myWidth, &myHeight, &nComponents, 4);
			}

			TextureDesc texDesc{};

			texDesc.name = texture->name;
			texDesc.bytesPerPixel = 4;
			texDesc.width = myWidth;
			texDesc.height = myHeight;
			texDesc.data = myPixels;

			images[i] = texDesc;
		}

		// Load pieces
		size_t primitivesAt = 0;

		for (size_t i = 0; i < data->meshes_count; i += 1)
		{
			cgltf_mesh* mesh = &data->meshes[i];

			ModelPiece* piece = &pieces[i];
			strncpy_s(piece->name, mesh->name, arrayCount(piece->name));

			piece->aabb = AABB::invertedInfinity();

			piece->primitiveCount = mesh->primitives_count;
			piece->primitives = &primitives[primitivesAt];
			primitivesAt += piece->primitiveCount;

			for (size_t i = 0; i < mesh->primitives_count; i += 1)
			{
				//
				// Load primitives
				//

				cgltf_primitive* primitive = &mesh->primitives[i];
				if (primitive->type != cgltf_primitive_type_triangles)
				{
					//Log(LogLevel_Error, "[GLTF PARSER]: unsupported primitive type");
					return result;
				}

				

				size_t vertexCount = primitive->attributes[0].data->count;
				//size_t vertexDataSize = vertexCount * sizeof(Vertex);

				std::vector<float> vertices;

				//auto vertices = new Vertex[vertexCount];
				//defer{ delete[] vertices; };
				
				// In case you didn't know, this is the syntax to create a pointer to a fixed size array
				float(*positions)[3] = nullptr;
				float(*texcoords)[2] = nullptr;
				float(*normals)[3] = nullptr;

				glm::vec3 pMin = glm::vec3(0);
				glm::vec3 pMax = glm::vec3(0);

				for (size_t i = 0; i < primitive->attributes_count; i += 1)
				{
					cgltf_attribute* attr = &primitive->attributes[i];
					if (attr->data->count != vertexCount)
					{
						//Log(LogLevel_Error, "[GLTF PARSER]: mismatched attribute counts");
						return result;
					}

					switch (attr->type)
					{
					case cgltf_attribute_type_position:
					{
						positions = GetAccessorPointer<float[3]>(attr->data);
						pMin = glm::vec3(attr->data->min[0], attr->data->min[1], attr->data->min[2]);
						pMax = glm::vec3(attr->data->max[0], attr->data->max[1], attr->data->max[2]);
					} break;

					case cgltf_attribute_type_texcoord:
					{
						texcoords = GetAccessorPointer<float[2]>(attr->data);
					} break;

					case cgltf_attribute_type_normal:
					{
						normals = GetAccessorPointer<float[3]>(attr->data);
					} break;
					}
				}

				std::vector<vertexType> myVertexLayout;
				if (positions)
				{
					myVertexLayout.push_back(vertexType::Position3);
				}

				if (normals)
				{
					myVertexLayout.push_back(vertexType::Normal3);
				}

				if (texcoords)
				{
					myVertexLayout.push_back(vertexType::TexCoord2);
				}

				for (size_t i = 0; i < vertexCount; i += 1)
				{
					if (positions)
					{
						vertices.push_back(positions[i][0]);
						vertices.push_back(positions[i][1]);
						vertices.push_back(positions[i][2]);

						//memcpy(&vertex->position, &positions[i], sizeof(positions[i]));
					}
					if (normals)
					{
						vertices.push_back(normals[i][0]);
						vertices.push_back(normals[i][1]);
						vertices.push_back(normals[i][2]);

						//memcpy(&vertex->normal, &normals[i], sizeof(normals[i]));
					}
						
					if (texcoords)
					{
						vertices.push_back(texcoords[i][0]);
						vertices.push_back(texcoords[i][1]);

						//memcpy(&vertex->texcoord, &texcoords[i], sizeof(texcoords[i]));
					}
				}

				piece->aabb = combine(piece->aabb, AABB::minMax(pMin, pMax));

				ModelPrimitive* myPrimitive = &piece->primitives[i];

				MeshDesc meshDesc{};

				meshDesc.vertexData = malloc(vertices.size() * sizeof(float));
				memcpy(meshDesc.vertexData, vertices.data(), vertices.size() * sizeof(float));
				meshDesc.vertexDataSize = vertices.size() * sizeof(float);

				meshDesc.vertexLayout = myVertexLayout;

				if (primitive->indices)
				{
					uint16_t* indices = GetAccessorPointer<uint16_t>(primitive->indices);

					size_t indexdataSize = primitive->indices->count * primitive->indices->stride;

					meshDesc.indexData = indices;
					meshDesc.indexDataSize = indexdataSize;
				
					if (primitive->indices->stride == 2)
					{
						meshDesc.indexBufferFormat = IndexBufferFormat::uint16;
					}
					else if (primitive->indices->stride == 4)
					{
						meshDesc.indexBufferFormat = IndexBufferFormat::uint32;
					}
					else
					{
						//log error
						return result;
					}
				}

				myPrimitive->meshDesc = meshDesc;
				
				myPrimitive->material.textureBaseColor = TextureDesc();
				myPrimitive->material.textureNormal = TextureDesc();
				myPrimitive->material.textureMetallicRoughness = TextureDesc();

				cgltf_texture* diffuse = nullptr;
				cgltf_texture* normal = nullptr;
				cgltf_material* material = primitive->material;
				if (material->has_pbr_metallic_roughness)
				{
					diffuse = material->pbr_metallic_roughness.base_color_texture.texture;
					myPrimitive->material.baseColorFactor = glm::vec3(material->pbr_metallic_roughness.base_color_factor[0],
						material->pbr_metallic_roughness.base_color_factor[1],
						material->pbr_metallic_roughness.base_color_factor[2]);
				}
				else if (material->has_pbr_specular_glossiness)
				{
					diffuse = material->pbr_specular_glossiness.diffuse_texture.texture;
					myPrimitive->material.baseColorFactor = glm::vec3(material->pbr_specular_glossiness.diffuse_factor[0],
						material->pbr_specular_glossiness.diffuse_factor[1],
						material->pbr_specular_glossiness.diffuse_factor[2]);
				}

				normal = material->normal_texture.texture;

				if (diffuse)
				{
					myPrimitive->material.textureBaseColor = images[GLTFImageIndex(data, diffuse->image)];
				}

				if (normal)
				{
					myPrimitive->material.textureNormal = images[GLTFImageIndex(data, normal->image)];
				}
			}
		}

		// Load nodes
		size_t* nodeMap = new size_t[data->nodes_count];
		defer{ delete[] nodeMap; };

		size_t nodeAt = 0;
		size_t childNodesAt = 0;
		for (size_t i = 0; i < data->nodes_count; i += 1)
		{
			cgltf_node* gltfNode = &data->nodes[i];

			size_t nodeIndex = nodeAt++;
			nodeMap[i] = nodeIndex;

			ModelNode* node = &nodes[nodeIndex];
			strncpy_s(node->name, gltfNode->name, arrayCount(node->name));

			if (gltfNode->parent)
			{
				node->parent = &nodes[GLTFNodeIndex(data, gltfNode->parent)];
			}

			node->childCount = gltfNode->children_count;
			if (node->childCount)
			{
				node->children = &childNodes[childNodesAt];
				childNodesAt += node->childCount;
			}

			for (size_t childIndex = 0; childIndex < node->childCount; childIndex += 1)
			{
				auto test = GLTFNodeIndex(data, gltfNode->children[childIndex]);

				node->children[childIndex] = &nodes[test];
			}

			glm::mat4 transform;
			memcpy(&transform, &gltfNode->matrix, sizeof(gltfNode->matrix));

			glm::vec3 t;
			glm::quat r;
			glm::vec3 s;

			t = glm::vec3(gltfNode->translation[0], gltfNode->translation[1], gltfNode->translation[2]);
			r = glm::quat(gltfNode->rotation[3], gltfNode->rotation[0], gltfNode->rotation[1], gltfNode->rotation[2]);
			s = glm::vec3(gltfNode->scale[0], gltfNode->scale[1], gltfNode->scale[2]);

			glm::mat4 TRS = glm::translate(t) * glm::mat4_cast(r) * glm::scale(s);

			node->transform = transform * TRS;

			if (gltfNode->mesh)
			{
				node->piece = &pieces[GLTFMeshIndex(data, gltfNode->mesh)];
			}
		}

		if (scene)
		{
			model->nodes = &childNodes[childNodesAt];

			for (size_t i = 0; i < scene->nodes_count; i += 1)
			{
				cgltf_node* gltfNode = scene->nodes[i];

				size_t nodeIndex = nodeMap[GLTFNodeIndex(data, gltfNode)];
				if (nodeIndex == SIZE_MAX)
				{
					continue;
				}
				model->nodes[model->nodeCount++] = &nodes[nodeIndex];
			}

			childNodesAt += model->nodeCount;
		}

		model->aabb = AABB::invertedInfinity();
		for (size_t i = 0; i < model->nodeCount; i += 1)
		{
			AABB nodeBounds = CalculateAABB(model->nodes[i]);
			model->aabb = combine(model->aabb, nodeBounds);
		}

		model->aabb.min = transformPoint(g_coordinateTransform, model->aabb.min);
		model->aabb.max = transformPoint(g_coordinateTransform, model->aabb.max);

		result = model;

		// stop the asset's memory from getting freed when we exit this scope
		memoryPool = nullptr;
	}

	return result;
}