#include "DXrenderer/Textures/Texture.h"


#include "Utils/Logger.h"

#include "External/lodepng/lodepng.h"

#define TINYEXR_IMPLEMENTATION
#include "External/TinyEXR/tinyexr.h"

#define STB_IMAGE_IMPLEMENTATION
#include <filesystem>

#include "External/stb/stb_image.h"

namespace DirectxPlayground
{
    namespace TextureUtils
    {
        bool ParsePNG(const std::string& filename, std::vector<byte>& buffer, UINT& w, UINT& h, DXGI_FORMAT& textureFormat)
        {
            std::vector<byte> bufferInMemory;
            lodepng::load_file(bufferInMemory, filename);
            UINT error = lodepng::decode(buffer, w, h, bufferInMemory);
            if (error)
            {
                LOG("PNG decoding error ", error, " : ", lodepng_error_text(error), "\n");
                return false;
            }
            textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            return true;
        }

        bool ParseEXR(const std::string& filename, std::vector<byte>& buffer, UINT& w, UINT& h, DXGI_FORMAT& textureFormat)
        {
            int width = 0, height = 0;
            float* out = nullptr;
            const char* err = nullptr;
            int ret = LoadEXR(&out, &width, &height, filename.c_str(), &err);
            if (ret != TINYEXR_SUCCESS)
            {
                if (err != nullptr)
                {
                    LOG("EXR decoding error ", err);
                    FreeEXRErrorMessage(err);
                }
                return false;
            }
            else
            {
                w = static_cast<UINT>(width);
                h = static_cast<UINT>(height);

                size_t imageSize = size_t(w) * size_t(h) * sizeof(float) * 4U;
                buffer.resize(imageSize);
                memcpy(buffer.data(), out, imageSize);

                textureFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;

                free(out);
            }
            return true;
        }

        bool ParseHDR(const std::string& filename, std::vector<byte>& buffer, UINT& w, UINT& h, DXGI_FORMAT& textureFormat)
        {
            int width = 0;
            int height = 0;
            int nComponents = 0;
            float* data = stbi_loadf(filename.c_str(), &width, &height, &nComponents, 0);
            if (data == nullptr)
            {
                LOG("HDR decoding error");
                return false;
            }
            if (nComponents != 3 && nComponents != 4)
            {
                LOG("HDR decoding error. Invalid number of color channels in the file");
                return false;
            }
            w = static_cast<UINT>(width);
            h = static_cast<UINT>(height);

            size_t imageSize = size_t(w) * size_t(h) * sizeof(float) * 4U;
            buffer.resize(imageSize);
            // [a_vorontcov] Well extension to 4 components because we want to use the texture as uav, and currently R32G32B32 format isn't supported.
            if (nComponents == 4)
            {
                memcpy(buffer.data(), data, imageSize);
            }
            else
            {
                float* bufferAsFloat = reinterpret_cast<float*>(buffer.data());
                for (size_t i = 0; i < size_t(w) * size_t(h); ++i)
                {
                    bufferAsFloat[i * 4 + 0] = data[i * 3 + 0];
                    bufferAsFloat[i * 4 + 1] = data[i * 3 + 1];
                    bufferAsFloat[i * 4 + 2] = data[i * 3 + 2];
                    bufferAsFloat[i * 4 + 3] = 1.0f;
                }
            }

            textureFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;

            stbi_image_free(data);

            return true;
        }
    }

    void Texture::Parse(const std::string& filename)
    {
        std::wstring extension{ std::filesystem::path(filename.c_str()).extension().c_str() };

        if (extension == L".png" || extension == L".PNG") // let's hope there won't be "pNg" or "PnG" etc
        {
            bool success = TextureUtils::ParsePNG(filename, mData, mWidth, mHeight, mFormat);
        }
        else if (extension == L".exr" || extension == L".EXR")
        {
            bool success = TextureUtils::ParseEXR(filename, mData, mWidth, mHeight, mFormat);
        }
        else if (extension == L".hdr" || extension == L".HDR")
        {
            bool success = TextureUtils::ParseHDR(filename, mData, mWidth, mHeight, mFormat);
        }
        else
        {
            assert("Unknown image format for parsing" && false);
        }
    }

    void Texture::Serialize(BinaryContainer& container)
    {
        
    }

    void Texture::Deserialize(BinaryContainer& container)
    {
        
    }
}
