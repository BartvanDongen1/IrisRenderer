cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 viewProjectionMatrix;
    
    float4 padding[12];
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;

    result.position = mul(viewProjectionMatrix, float4(position.xyz, 1.0f));
    result.color = color;
    
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}