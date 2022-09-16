
Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 viewProjectionMatrix;
    float4x4 modelMatrix;

    float4 viewPosition;
    
    float4 color;
    
    float4x4 depthBiasMVPMatrix;
    float4 lightPosition;
    
    int materialProperties;
    
    float padding[3];
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    
    float4 worldPosition : TEXCOORD1;
    float4 shadowCoord : TEXCOORD2;
};

PSInput VSMain(float4 position : POSITION, float3 normal : NORMAL, float2 texCoord : TEXCOORD)
{
    PSInput result;
    
    float4x4 MVP = mul(viewProjectionMatrix, modelMatrix);

    result.position = mul(MVP, float4(position.xyz, 1.0f));
    
    result.normal = normal;
    result.uv = texCoord;
    
    float4 worldPosition = mul(modelMatrix, float4(position.xyz, 1.0f));
    
    result.worldPosition = worldPosition;
    result.shadowCoord = mul(depthBiasMVPMatrix, worldPosition);
    
    return result;
}

#define MIN_BIAS 0.0
#define MAX_BIAS 0.01
#define BIAS_MULTIPLIER 0.005

static const float2 PoissonDisk[16] =
{
    { -0.94201624, -0.39906216 },
    { 0.94558609, -0.76890725 },
    { -0.094184101, -0.92938870 },
    { 0.34495938, 0.29387760 },
    { -0.91588581, 0.45771432 },
    { -0.81544232, -0.87912464 },
    { -0.38277543, 0.27676845 },
    { 0.97484398, 0.75648379 },
    { 0.44323325, -0.97511554 },
    { 0.53742981, -0.47373420 },
    { -0.26496911, -0.41893023 },
    { 0.79197514, 0.19090188 },
    { -0.24188840, 0.99706507 },
    { -0.81409955, 0.91437590 },
    { 0.19984126, 0.78641367 },
    { 0.14383161, -0.14100790 }
};

float calculateShadow(const float4 shadowPosition, const float3 surfaceNormal, float3 lightDirection)
{
    // perspective divide to normalize vector
    float3 projectedCoords = shadowPosition.xyz / shadowPosition.w;
    
    //go from range [-1,1] to [0,1]
    projectedCoords.xy = projectedCoords.xy * 0.5f + 0.5f;
    projectedCoords.y = 1 - projectedCoords.y;
    
    if (projectedCoords.z > 1.0 || projectedCoords.x < 0 || projectedCoords.x > 1 || projectedCoords.y < 0 || projectedCoords.y > 1)
    {
        return 0;
    }
    
    // calculate bias
    float bias = BIAS_MULTIPLIER * tan(acos(dot(surfaceNormal, lightDirection)));
    bias = clamp(bias, MIN_BIAS, MAX_BIAS);
    
    // PCF filtering
    float width; float height;
    g_texture.GetDimensions(width, height);
    float2 textureSize = float2(1.0f / width, 1.0f / height);
    
    float shadowAccumulation = 0.0f;
    
    for (int i = 0; i < 16; i++)
    {
        float2 offset = PoissonDisk[i] * textureSize * 0.5;
        
        float2 UV = projectedCoords.xy + offset;
        float closestDepth = g_texture.Sample(g_sampler, UV).x;;
        float currentDepth = projectedCoords.z;
        
        shadowAccumulation += currentDepth > closestDepth - bias ? 1.0f : 0.0f;
    }
    
    return shadowAccumulation / 16.0f;
    
    
    //float shadowAccumulation1 = 0.0f;
    //float shadowAccumulation2 = 0.0f;
    //float shadowAccumulation3 = 0.0f;
    //float shadowAccumulation4 = 0.0f;
    
    //float currentDepth = projectedCoords.z;
    //for (int i = 0; i < 16; i += 4)
    //{
    //    float2 offset1 = PoissonDisk[i] * textureSize * 0.5f;
    //    float closestDepth1 = g_texture.Sample(g_sampler, projectedCoords.xy + offset1).x;
       
    //    shadowAccumulation1 += currentDepth > closestDepth1 - bias ? 1.0f : 0.0f;

    //    float2 offset2 = PoissonDisk[i + 1] * textureSize * 0.5f;
    //    float closestDepth2 = g_texture.Sample(g_sampler, projectedCoords.xy + offset2).x;
        
    //    shadowAccumulation2 += currentDepth > closestDepth2 - bias ? 1.0f : 0.0f;
        
    //    float2 offset3 = PoissonDisk[i + 2] * textureSize * 0.5f;
    //    float closestDepth3 = g_texture.Sample(g_sampler, projectedCoords.xy + offset3).x;
       
    //    shadowAccumulation3 += currentDepth > closestDepth3 - bias ? 1.0f : 0.0f;
        
    //    float2 offset4 = PoissonDisk[i + 3] * textureSize * 0.5f;
    //    float closestDepth4 = g_texture.Sample(g_sampler, projectedCoords.xy + offset4).x;
       
    //    shadowAccumulation4 += currentDepth > closestDepth4 - bias ? 1.0f : 0.0f;
    //}
    
    //return ((shadowAccumulation1 + shadowAccumulation2) + (shadowAccumulation3 + shadowAccumulation4)) * 0.0625;
}

bool getSpecular()
{
    return (bool) (materialProperties & (0x1 << 31));
}

#define AMBIENT_MULT 0.6f
#define SHADOW_MULT 0.4f

float4 PSMain(PSInput input) : SV_TARGET
{    
    float4 myColor = color;
    float3 lightDirection = normalize(input.worldPosition.xyz - lightPosition.xyz);
    
    // diffuse
    float diffuse = max(dot(-lightDirection, input.normal), 0.0);

    // specular
    float spec = 0.0;
    if (getSpecular())
    {
        float3 viewDir = normalize(viewPosition.xyz - input.worldPosition.xyz);
        float3 halfwayDir = normalize(-lightDirection + viewDir);
        spec = pow(max(dot(input.normal, halfwayDir), 0.0), 64.0);
    }
    
    // shadows
    float shadow = calculateShadow(input.shadowCoord, input.normal, lightDirection);
    
    //float lightIntensity = AMBIENT_MULT + ((1 - shadow) * SHADOW_MULT) + (diffuse * DEFUSE_MULT);
    float lightIntensity = AMBIENT_MULT + (((1 - shadow) * (diffuse + spec)) * SHADOW_MULT);

    return myColor * lightIntensity;
}