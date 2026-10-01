#include "stb.hpp"
#include "common.hpp"

#include <stdexcept>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

STBImage::STBImage() {}

STBImage::STBImage(std::string path)
{
    mImage = stbi_load(path.c_str(), &mWidth, &mHeight, &mChannels, 0);
    if (mImage == NULL)
    {
        throw std::runtime_error("Unable to load file");
    }
}

Color STBImage::get(int y, int x)
{
    y = CLAMP(y, 0, mHeight - 1);
    x = CLAMP(x, 0, mWidth - 1);

    int                  widthBytes = mWidth * mChannels;
    const unsigned char *offset     = mImage + y * widthBytes + x * mChannels;
    return Color(Vector(offset[0] / 255.0, offset[1] / 255.0, offset[2] / 255.0));
}

Color STBImage::getUv(float u, float v)
{
    return get(v * (mHeight - 1), u * (mWidth - 1));
}

unsigned char STBImage::get(int y, int x, int color)
{
    return get(y, x)[color];
}

float STBImage::getDbl(int y, int x, int color)
{
    return get(y, x, color) / 255.0;
}

void STBImage::free()
{
    // Destructors and copy constructors caused problems with accidental freeing
    stbi_image_free((void *)mImage);
}
