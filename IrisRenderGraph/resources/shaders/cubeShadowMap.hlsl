cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 viewProjectionMatrix;
    float4x4 modelMatrix;
    
    float4 padding[8];
};

struct PSInput
{
    float4 position : SV_POSITION;
};

PSInput VSMain(float4 position : POSITION, float2 texCoord : TEXCOORD)
{
    PSInput result;

    float4x4 MVP = mul(viewProjectionMatrix, modelMatrix);

    result.position = mul(MVP, float4(position.xyz, 1.0f));
    
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.position.z, input.position.z, input.position.z, 1.0f);
}