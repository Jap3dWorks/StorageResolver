#pragma once
#include <cstdint>
#include <vector>


enum class ETextureChannels : uint8_t
{
    // 8 bit
    // R=1,G=2,B=4,A=8
    kR=1,
    kRG=3,
    kRGB=7,
    kRGBA=15,
    // 16 bit
    // d_16=16,
    kR16=17,
    kRG16=19,
    kRGB16=23,
    kRGBA16=31,
    // 32 bit
    // d_32=32,
    kR32=33,
    kRG32=35,
    kRGB32=39,
    kRGBA32=47,
};

ETextureChannels operator|(ETextureChannels a, ETextureChannels b)
{
    return static_cast<ETextureChannels>(
        static_cast<uint8_t>(a) | static_cast<uint8_t>(b)
    );
}

ETextureChannels operator&(ETextureChannels a, ETextureChannels b)
{
    return static_cast<ETextureChannels>(
        static_cast<uint8_t>(a) & static_cast<uint8_t>(b)
    );
}

struct WTextureStruct{
    std::vector<uint8_t> data;
    ETextureChannels channels;
    uint32_t width;
    uint32_t height;    
};
