#pragma once

#include "Utils/Asset.h"

#include <filesystem>
#include <fstream>

#include "BinaryContainer.h"

namespace DirectxPlayground::AssetSystem
{
void Load(const std::string& assetPath, Asset& asset)
{
    // <assetPath> == \Repos\DXRplayground\DXRplayground\Assets\Models\FlightHelmet\glTF\FlightHelmet.gltf
    using namespace std;

    filesystem::path currentAssetPath{ assetPath };

    std::filesystem::path filename = currentAssetPath.stem(); // FlightHelmet
    std::filesystem::path projectAssetsPath{ ASSETS_DIR }; // \Repos\DXRplayground\DXRplayground\Assets
    std::filesystem::path relative = std::filesystem::relative(assetPath, projectAssetsPath); // Models\FlightHelmet\glTF\FlightHelmet.gltf
    std::filesystem::path parentPath = projectAssetsPath.parent_path().parent_path(); // <-- due to // in assets assetPath. \Repos\DXRplayground\DXRplayground (to create tmp there)
    //auto assetPath = ASSETS_DIR + std::string("Models//FlightHelmet//glTF//FlightHelmet.gltf");
    std::filesystem::path binAssetPath{ parentPath.string() + std::string("//tmp//") + relative.string() }; // <---- extension Binary path
    if (!std::filesystem::exists(binAssetPath))
    {
        BinaryContainer out{};
        std::filesystem::path parentDir = binAssetPath.parent_path();
        if (!std::filesystem::exists(parentDir))
        {
            std::filesystem::create_directories(parentDir);
        }

        std::vector<int> src{ -5, 12, 82, 144, -57 };
        out << src;

        std::string ololosha = "trololosha";
        out << ololosha;

        Structololosha stro{ 12, 24 };
        out << stro;

        out.Close();

        std::ofstream outFile(binAssetPath.string(), std::ios::out | std::ios::binary);
        outFile.write(out.GetData(), out.GetLastPointerOffset());
        outFile.close();

        //////
        std::ifstream inFile(binAssetPath.string(), std::ios::in | std::ios::binary);
        size_t byteSize = 0;
        inFile.read((char*)&byteSize, sizeof(std::size_t));
        char* buf = new char[byteSize];
        inFile.read(buf, byteSize);
        inFile.close();

        BinaryContainer in{ buf, byteSize };
        std::vector<int> resStream;
        in >> resStream;

        std::string resString;
        in >> resString;

        Structololosha oStro;
        in >> oStro;
        in.Close();
    }
    else
    {
        filesystem::file_time_type lastModificationTime = filesystem::last_write_time(binAssetPath);
        size_t lmtSinceEpoch = static_cast<size_t>(lastModificationTime.time_since_epoch().count());

    }
}
}
