#ifndef LIGHTING_HLSL
#define LIGHTING_HLSL

float4 sRGBtoRGB(float4 c)
{
    return float4(pow(c.rgb, 2.2f), c.a);
}

float4 RGBtosRGB(float4 c)
{
    return float4(pow(c.rgb, 1.0f / 2.2f), c.a);
}

#endif