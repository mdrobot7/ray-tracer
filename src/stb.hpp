#pragma once

#include <string>

#include "color.hpp"
#include "stb_image.h"
#include "stb_image_write.h"

class STBImage
{
  public:
    const unsigned char *mImage;
    int                  mWidth, mHeight, mChannels;

    STBImage();
    STBImage(std::string path);

    /**
     * @brief Get a pixel from the image.
     */
    Color get(int y, int x);

    /**
     * @brief Get a pixel from the image using
     * texture (u, v) coordinates. u and v are in the
     * range [0, 1.0].
     */
    Color getUv(float u, float v);

    /**
     * @brief Get a color byte [0-255] from the image.
     * Image is in RGB(A) order.
     */
    unsigned char get(int y, int x, int color);

    /**
     * @brief Get a color float [0-1.0] from the image.
     * Image is in RGB(A) order.
     */
    float getDbl(int y, int x, int color);

    /**
     * @brief Frees the memory allocated for the image.
     */
    void free();
};
