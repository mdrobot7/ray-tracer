#pragma once

#include "bvh.hpp"
#include "ray.hpp"
#include "scene.hpp"
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class Render
{
  public:
    Render(Scene &scene, BoundingVolumeHierarchy &bvh, int width, int height, int antiAliasingLevel,
           int jobs, int maxBounces);
    ~Render();

    /**
     * @brief Render the scene.
     *
     * @return int 0 on success, nonzero otherwise
     */
    int run();

    /**
     * @brief Saves the rendered framebuffer to two PPM
     * files: filename.ppm (binary) and filename.txt.ppm
     * (ASCII).
     *
     * @param filename
     * @return int
     */
    int save(std::string filename);

  private:
    Scene &mScene;

    BoundingVolumeHierarchy &mBvh;

    int        mWidth, mHeight, mAntiAliasingLevel;
    uint8_t   *mFb;
    std::mutex mFbLock;

    int                      mJobs;
    std::vector<std::thread> mThreads;
    int                      mNextPixelX, mNextPixelY;
    std::mutex               mNextPixelLock;
    bool                     mKillThreads;

    int mMaxBounces; // Max bounces per ray before we call it black

    /**
     * @brief Job for an individual thread in the thread
     * pool. Renders the next pixel in the queue and puts
     * the result in the framebuffer.
     */
    void renderPixel();

    /**
     * @brief Get a pointer to the pixel in the framebuffer
     * specified by x, y.
     */
    uint8_t *getPixel(int y, int x);
};
