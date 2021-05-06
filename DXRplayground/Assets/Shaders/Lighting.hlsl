#ifndef LIGHTING_HLSL
#define LIGHTING_HLSL

#define PI 3.14159265358979323846f

float4 sRGBtoRGB(float4 c)
{
    return float4(pow(c.rgb, 2.2f), c.a);
}

float4 RGBtosRGB(float4 c)
{
    return float4(pow(c.rgb, 1.0f / 2.2f), c.a);
}

float3 BlinnPhong(float3 lightDir, float3 lightColor, float3 eyePos, float3 pos, float3 n)
{
    float3 v = normalize(pos - eyePos);
    float3 h = normalize(-lightDir + v);
    float d = max(0.0f, dot(n, -lightDir));
    float s = pow(max(0.0f, dot(n, h)), 64);

    return lightColor * (d + s) +float3(0.1f, 0.1f, 0.1f);
}

// Trowbridge-Reitz GGX - NormalDistribution
// n - normal, h - halfvector, roughness - roughness
float GGXDistribution(float3 n, float3 h, float roughness)
{
    float a = roughness * roughness;
    float asqr = a * a;
    float NdotH = max(dot(n, h), 0.0f);
    float NdotHsqr = NdotH * NdotH;

    float denom = NdotHsqr * (asqr - 1.0f) + 1.0f;
    denom = denom * denom * PI;

    return asqr / denom;
}

// Schlick-GGX. Geometry function + Smith method
// k is a remapping of a based on whether we're using the geometry function for either direct lighting or IBL lighting
float GGXGeometrySchlick(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f; // remapping of roughness for direct calculations (for ibl roug^2 / 2.0)

    float denom = NdotV * (1.0f - k) + k;
    return NdotV / denom;
}

float GeometrySmith(float3 n, float3 v, float3 l, float roughness)
{
    float NdotV = max(dot(n, v), 0.0f);
    float NdotL = max(dot(n, l), 0.0f);
    float ggx1 = GGXGeometrySchlick(NdotV, roughness);
    float ggx2 = GGXGeometrySchlick(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 NormalIncidenceFresnel(float3 color, float metalness)
{
    float3 f0 = float3(0.04f, 0.04f, 0.04f); // Approximation for non dielectrics
    return lerp(f0, color, metalness);
}

float3 FresnelSchlick(float HdotV, float3 f0)
{
    return f0 + (1.0f - f0) * pow(max(1.0f - HdotV, 0.0f), 5.0f);
}

float3 FresnelSchlick(float3 h, float3 v, float3 f0)
{
    float HdotV = dot(h, v);
    return FresnelSchlick(HdotV, f0);
}


#endif