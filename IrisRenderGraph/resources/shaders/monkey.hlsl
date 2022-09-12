
Texture2D g_texture1 : register(t0);
Texture2D g_texture2 : register(t1);
Texture2D g_texture3 : register(t2);

SamplerState g_sampler : register(s0);

cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 viewProjectionMatrix;
    float4x4 modelMatrix;

    float4 padding[8];
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

    //result.position = position;

    float4x4 MVP = mul(viewProjectionMatrix, modelMatrix);

    result.position = mul(MVP, float4(position.xyz, 1.0f));
    result.normal = normal;
    result.uv = texCoord;
    
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    //return float4(1,1,1,1);

    //return g_texture1.Sample(g_sampler, input.uv);
    //return g_texture2.Sample(g_sampler, input.uv);
    return g_texture3.Sample(g_sampler, input.uv);
}