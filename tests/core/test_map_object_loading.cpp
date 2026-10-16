#include "stdafx.h"
#include "doctest.h"
#include "Engine/Object/ZzzObject.h"
#include "UI/NewUI/HUD/NewUIMiniMap.h"
#include "World/MapInfra/MapManager.h"
#include "Render/Terrain/ZzzLodTerrain.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>

TEST_CASE("Malformed Arena model indices do not enter the object list [world][objects]")
{
    // The original Arena contains model index -515. Loading it must not create
    // an object that later indexes before the model array during rendering.
    const auto path = std::filesystem::temp_directory_path()
        / ("mu-invalid-model-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".obj");
    std::array<OBJECT*, 256> originalHeads;
    for (std::size_t block = 0; block < originalHeads.size(); ++block)
        originalHeads[block] = ObjectBlock[block].Head;

    const unsigned char version = 0;
    const short count = 2;
    const std::array<short, 2> invalidTypes = {-515, static_cast<short>(MAX_MODELS)};
    const std::array<float, 7> placement = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f};
    {
        std::ofstream file(path, std::ios::binary);
        REQUIRE(file.is_open());
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const short type : invalidTypes)
        {
            file.write(reinterpret_cast<const char*>(&type), sizeof(type));
            file.write(reinterpret_cast<const char*>(placement.data()), sizeof(placement));
        }
    }
    auto filename = path.wstring();
    const int map = OpenObjects(filename.data());
    std::filesystem::remove(path);
    CHECK(map == 0);
    for (std::size_t block = 0; block < originalHeads.size(); ++block)
        CHECK(ObjectBlock[block].Head == originalHeads[block]);
}

TEST_CASE("Arena blocks minimap opening even when its image and show flag exist [world][minimap]")
{
    const int previousWorld = gMapManager.WorldActive;
    SEASON3B::CNewUIMiniMap minimap;
    gMapManager.WorldActive = WD_0LORENCIA;
    minimap.m_bSuccess = true;
    minimap.Show(true);
    CHECK(minimap.IsVisible());
    CHECK(minimap.IsEnabled());

    gMapManager.WorldActive = WD_6STADIUM;
    CHECK_FALSE(minimap.IsAvailable());
    CHECK_FALSE(minimap.IsVisible());
    CHECK_FALSE(minimap.IsEnabled());
    CHECK_FALSE(minimap.Render());
    minimap.OpenningProcess();
    CHECK_FALSE(minimap.IsVisible());

    minimap.Show(true);
    minimap.LoadImages(L"World7");
    CHECK_FALSE(minimap.m_bSuccess);
    CHECK_FALSE(minimap.IsVisible());
    gMapManager.WorldActive = previousWorld;
}

TEST_CASE("Full Arena height file loads every height sample [world][terrain]")
{
    const auto assetRoot = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() / "src/bin";
    const auto heightPath = assetRoot / "Data/World7/TerrainHeight.OZB";
    constexpr std::size_t pixelCount = 256 * 256;
    constexpr std::size_t headerSize = 4 + 1080;
    REQUIRE(std::filesystem::file_size(heightPath) == headerSize + pixelCount);
    std::array<unsigned char, pixelCount> pixels;
    {
        std::ifstream file(heightPath, std::ios::binary);
        file.seekg(headerSize);
        file.read(reinterpret_cast<char*>(pixels.data()), pixels.size());
        REQUIRE(file.gcount() == pixelCount);
    }
    const auto previousDirectory = std::filesystem::current_path();
    const int previousWorld = gMapManager.WorldActive;
    std::filesystem::current_path(assetRoot);
    gMapManager.WorldActive = WD_6STADIUM;
    wchar_t filename[] = L"World7\\TerrainHeight.bmp";
    const bool loaded = OpenTerrainHeight(filename);
    std::filesystem::current_path(previousDirectory);
    gMapManager.WorldActive = previousWorld;
    REQUIRE(loaded);
    for (std::size_t pixel = 0; pixel < pixels.size(); ++pixel)
        CHECK(BackTerrainHeight[pixel] == static_cast<float>(pixels[pixel]) * 1.5f);
}
