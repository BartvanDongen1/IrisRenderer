
Texture2D g_texture : register(t0);

SamplerState g_sampler : register(s0);

cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 viewProjectionMatrix;

    float4 padding[12];
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

PSInput VSMain(float4 position : POSITION, float3 normal : NORMAL, float2 texCoord : TEXCOORD)
{
    PSInput result;

    result.position = mul(viewProjectionMatrix, float4(position.xyz, 1.0f));
    result.position = result.position.xyww;
    result.normal = normal;
    result.uv = texCoord;
    
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv);
}