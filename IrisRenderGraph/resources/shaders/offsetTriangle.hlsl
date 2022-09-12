
cbuffer SceneConstantBuffer : register(b0)
{
    float4 offset;
    float4 padding[15];
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

PSInput VSMain(float4 position : POSITION, float2 texCoord : TEXCOORD)
{
    PSInput result;

    result.position = position + offset;
    result.uv = texCoord;
    
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.uv[0], 0.0f , input.uv[1], 1.0f);
}