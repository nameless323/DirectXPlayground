RWTexture2DArray<float4> cubemaps[64] : register(u0, space2);
Texture2D<float4> Textures[10000] : register(t0);

struct CbTIndices
{
    uint CubemapIndex;
    uint EnvMapIndex;
};
ConstantBuffer<CbTIndices> cbTIndices : register(b0);

[numthreads(32, 32, 1)]
void cs(uint3 tId : SV_DispatchThreadID)
{
    cubemaps[cbTIndices.CubemapIndex][tId] = float4(tId.z / 6.0, 0, 0, 1);
}