#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <stb_image.h>

#include "Artifice/Core/Core.h"
#include "Artifice/Core/Log.h"
#include "Artifice/Utils/Timer.h"

struct ImageData
{
    uint32 Width;
    uint32 Height;
    std::vector<uint8> Data;
};
struct ImageDataHDR2
{
    uint32 Width;
    uint32 Height;
    std::vector<uint16> Data;
};

static std::vector<char> ReadFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary); //start reading from end and as binary

    AR_CORE_ASSERT(file.is_open(), "Failed to open file %s", filename.c_str());

    //get file size and alloacte space in buffer
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> result(fileSize);
    //go to beginning of file and fill buffer
    file.seekg(0);
    file.read(result.data(), fileSize);

    file.close();

    return result;
}

static ImageData ReadImage(const std::string& path)
{    
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    stbi_uc* data = nullptr;

    data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    AR_CORE_ASSERT(data, "Failed to load image!");

    uint32 size = width * height * 4;

    ImageData result;
    result.Width = width;
    result.Height = height;
    result.Data.resize(size);
    memcpy(result.Data.data(), data, size);

    stbi_image_free(data);

    return result;
}

static ImageData ReadImageHDR(const std::string& path)
{    
    Timer timer;
    
    int width, height, channels;
    stbi_set_flip_vertically_on_load(0);
    float* data = nullptr;
    data = stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    AR_CORE_ASSERT(data, "Failed to load image!");

    uint32 size = width * height * 4 * sizeof(float);

    ImageData result;
    result.Width = width;
    result.Height = height;
    result.Data.resize(size);

    memcpy(result.Data.data(), data, size);
    stbi_image_free(data);

    AR_CORE_INFO("Loaded hdr image %s in %f ms", path.c_str(), timer.ElapsedMillis());

    return result;
}

static uint16 ConvertFloatToHalfFloat(float f)
{
    uint32_t x = *((uint32_t*)&f);
    uint16_t h = ((x >> 16) & 0x8000) | ((((x & 0x7f800000) - 0x38000000) >> 13) & 0x7c00) | ((x >> 13) & 0x03ff);
    return h;
}

static ImageDataHDR2 ReadImageHDR2(const std::string& path)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(0);
    float* data = nullptr;
    data = stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    AR_CORE_ASSERT(data, "Failed to load image!");

    uint32 size = width * height * 4;

    std::vector<float> float_data(size);
    memcpy(float_data.data(), data, size * sizeof(float));
    stbi_image_free(data);

    ImageDataHDR2 result;
    result.Width = width;
    result.Height = height;
    result.Data.resize(size);

    for (uint32 i = 0; i < size; i++)
    {
        result.Data[i] = ConvertFloatToHalfFloat(float_data[i]);
    }

    return result;
}