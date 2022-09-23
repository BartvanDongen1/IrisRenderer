#pragma once
#include <vector>
#include <Windows.h>

// -------------------------
// render target
// -------------------------

enum class DX12Format
{
	R32G32B32A32_Float,
	R8G8B8A8_Unorm,
	D32_Float
};

enum class RenderTargetType
{
	RTV,
	DSV
};

struct RenderTargetDesc
{
	//DX12Format format;
	//RenderTargetType type;

	bool shaderReadable{ false };

	int size[2]{ 1, 1 };

	//bool isDepthStencil;
};

// -------------------------
// descriptor heap
// -------------------------

enum class descriptorType
{
	CBV_SRV_UAV,
	Sampler,
	RTV,
	DSV
};

enum class descriptorFlag
{
	None,
	Shader_visible
};

struct DescriptorHeapDesc
{
	descriptorType type;
	descriptorFlag flags{ descriptorFlag::None };

	int descriptorCount{ 1 };
};

// -------------------------
// rootSignature
// -------------------------

enum class descriptorRangeType
{
	CBV,
	SRV,
	Sampler
};

enum class shaderVisibility
{
	Pixel,
	Vertex
};

enum class rootSignatureFlags
{
	allowInputAssamblerInputLayout = 0x1,
	denyHullShaderAccess = 0x2,
	denyDomainShaderAccess = 0x4,
	denyGeometryShaderAccess = 0x8
};

// -------------------------
// pipeline
// -------------------------

enum class vertexType
{
	Position3,
	TexCoord2,
	Normal3
};

struct ConstBufferDesc
{
	const char* name{ "default" };
	size_t size;

	void* data;

	const char* graphBufferName{ "" };
};

struct TextureDesc
{
	const char* name{ "default" };

	int width{ 0 };
	int height{ 0 };
	int bytesPerPixel{ 0 };

	void* data{ nullptr };

	const char* renderTargetName{ "" };
	bool useDepthBuffer{ false };
};

struct SamplerDesc
{
	const char* name{ "default" };

	/* ADD STUFF TO THIS LATER */
};

struct PipelineObjectDesc
{
	LPCWSTR vertexShader;
	LPCWSTR pixelShader;

	std::vector<vertexType> vertexLayout;

	std::vector<ConstBufferDesc> constBuffers;
	std::vector<TextureDesc> textures;
	std::vector<SamplerDesc> samplers;

	rootSignatureFlags rootSignatureFlags;

	bool cullFront{ false };
};

// -------------------------
// vertexBuffer
// -------------------------

enum class IndexBufferFormat
{
	uint16,
	uint32
};

struct MeshDesc
{
	void* vertexData{ nullptr };
	size_t vertexDataSize{ 0 };

	std::vector<vertexType> vertexLayout;

	void* indexData{ nullptr };
	size_t indexDataSize{ 0 };

	IndexBufferFormat indexBufferFormat{ IndexBufferFormat::uint16 };
};