
Texture2D g_texture : register(t0);
SamplerComparisonState g_sampler : register(s0);

cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 viewProjectionMatrix;
    float4x4 modelMatrix;

    float4 color;
    
    float4x4 depthBiasMVPMatrix;
    
    float4 padding[3];
};

struct PSInput
{
    float4 position : SV_POSITION;
    //float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    
    float4 shadowCoord : TEXCOORD1;
};

PSInput VSMain(float4 position : POSITION, float2 texCoord : TEXCOORD)
{
    PSInput result;
    
    float4x4 MVP = mul(viewProjectionMatrix, modelMatrix);

    result.position = mul(MVP, float4(position.xyz, 1.0f));
    result.uv = texCoord;
    
    float4 test = mul(modelMatrix, float4(position.xyz, 1.0f));
    result.shadowCoord = mul(depthBiasMVPMatrix, test);
    
    return result;
}

//float2 calculateShadow(const float4 shadowPosition)
//{
//    // perspective divide to normalize vector
//    float3 projectedCoords = shadowPosition.xyz / shadowPosition.w;
    
//    //go from range [-1,1] to [0,1]
//    //projectedCoords = projectedCoords * 0.5f + 0.5f;
    
//    float closestDepth = g_texture.Sample(g_sampler, projectedCoords.xy).x;
//    float currentDepth = projectedCoords.z;
    
//    if (projectedCoords.z > 1.0)
//    {
//        //return 0;
//    }
    
//    //if (projectedCoords.x < 0 || projectedCoords.x > 1 || projectedCoords.y < 0 || projectedCoords.y > 1)
//    //{
//    //    return 1;
//    //}
    
//    //return 0;
    
//    // compare depth values
//    float shadow = currentDepth > closestDepth ? 1.0 : 0.0;
    
//    return float2(currentDepth, currentDepth);
//}

float calculateShadow2(const float4 shadowPosition)
{
    // perspective divide to normalize vector
    float3 projectedCoords = shadowPosition.xyz / shadowPosition.w;
    
    //go from range [-1,1] to [0,1]
    //projectedCoords = projectedCoords * 0.5f + 0.5f;
    
    if (projectedCoords.z > 1.0)
    {
        return 1;
    }
    
    if (projectedCoords.x < 0 || projectedCoords.x > 1 || projectedCoords.y < 0 || projectedCoords.y > 1)
    {
        return 1;
    }
    
    float returnValue = g_texture.SampleCmpLevelZero(g_sampler, projectedCoords.xy, projectedCoords.z);
    
    return returnValue;
    
    //float closestDepth = g_texture.Sample(g_sampler, projectedCoords.xy).x;
    //float currentDepth = projectedCoords.z;
    
   
    
    //// compare depth values
    //float shadow = currentDepth > closestDepth ? 1.0 : 0.0;
    
    //return shadow;
}


float4 PSMain(PSInput input) : SV_TARGET
{    
    //return float4(input.shadowCoord.xyz / 100, 1.0);
    
    //float test = linearize_depth(input.shadowCoord.z, 2, 1);
    //float test = depth2(input.shadowCoord.z, 0.1, 200);
    
    //return float4(test, test, test, 1.0);
    
    //float test = g_texture.Sample(g_sampler, input.shadowCoord.xy);
    //test = linearize_depth(test, 8, 2);
    
    //if (test == 0)
    //{
    //    return float4(1, 0, 0, 1.0f);
    //}
    
    //return float4(test, test, test, 1.0f);
    
    //if ((input.shadowCoord.z % 2) < 1)
    //{
    //    return color;
    //}
    //else
    //{
    //    return float4(0, 0, 0, 1);
    //}
    
    //g_texture.Sample(g_sampler, input.uv);
    
    float4 myColor = color;
    
    float shadow = calculateShadow2(input.shadowCoord);

    float min = 0.99f;
    float max = 1.f;
    
    float temp = ((shadow - min) / (max - min));
    
    myColor *= float4(temp, temp, temp, 1.0f);
  
    return myColor;
    
    //return color;
    
    //if (calculateShadow(input.shadowCoord).x > 1.f)
    //{
    //    return float4(1, 0, 0, 1);
    //}
    
    //return float4(calculateShadow(input.shadowCoord), 1, 1);
    
    //return g_texture.Sample(g_sampler, input.uv);

}