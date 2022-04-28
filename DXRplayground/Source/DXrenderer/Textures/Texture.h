#pragma once

#include "Utils/Asset.h"

#include <vector>
#include <wrl.h>
#include <dxgi1_6.h>

namespace DirectxPlayground
{
namespace TextureUtils
{
    bool ParsePNG(const std::string& filename, std::vector<byte>& buffer, UINT& w, UINT& h, DXGI_FORMAT& textureFormat);
    bool ParseEXR(const std::string& filename, std::vector<byte>& buffer, UINT& w, UINT& h, DXGI_FORMAT& textureFormat);
    bool ParseHDR(const std::string& filename, std::vector<byte>& buffer, UINT& w, UINT& h, DXGI_FORMAT& textureFormat);
}

class Texture : public Asset
{
public:
    Texture() : mWidth(-1), mHeight(-1), mFormat(DXGI_FORMAT_UNKNOWN)
    {}

    void Parse(const std::string& filename) override;
    void Serialize(BinaryContainer& container) override;
    void Deserialize(BinaryContainer& container) override;
    size_t GetVersion() const override
    {
        return AssetSerializationVersion;
    }

    const std::vector<byte>& GetData() const
    {
        return mData;
    }

    UINT GetWidth() const
    {
        return mWidth;
    }

    UINT GetHeight() const
    {
        return mHeight;
    }

    DXGI_FORMAT GetFormat() const
    {
        return mFormat;
    }
private:
    inline static constexpr size_t AssetSerializationVersion = 0;

    std::vector<byte> mData;
    UINT mWidth;
    UINT mHeight;
    DXGI_FORMAT mFormat;
};
}
