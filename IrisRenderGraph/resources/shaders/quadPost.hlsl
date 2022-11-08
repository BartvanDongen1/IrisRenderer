
Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

PSInput VSMain(float4 position : POSITION, float2 texCoord : TEXCOORD)
{
    PSInput result;

    result.position = position;
    result.uv = texCoord;
    
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 inputUV = input.uv;
    float vignetteIntensity = 15;
    float amount = 0.0;
    
    float2 correctedUV = inputUV * (1.0 - inputUV.yx);
    float vig = correctedUV.x * correctedUV.y * vignetteIntensity;
    vig = pow(vig, 0.25);
    
    float4 output = float4(0, 0, 0, 1);
    output.r = g_texture.Sample(g_sampler, float2(inputUV.x + amount, inputUV.y)).r;
    output.g = g_texture.Sample(g_sampler, inputUV).g;
    output.b = g_texture.Sample(g_sampler, float2(inputUV.x - amount, inputUV.y)).b;

    output *= (1.0 - amount * 0.5);
	
    return vig * output;
}