#pragma once

#include "vector.hpp"
#include <vector>

class Perlin
{
  public:
    Perlin();

    /**
     * @brief Get the Perlin noise value for a particular location.
     * Can be used to scale texture brightness, modify textures,
     * or generate terrain.
     *
     * Outputs a value in the range [0, 1].
     */
    float get(const Vector &vec);

  private:
    // Good for objects on the 10s to low 100s scale, smaller objects = larger frequency
    static constexpr float sFrequency = 0.2;

    static constexpr int sNumPoints = 256;
    std::vector<Vector>  mVectors;
    std::vector<int>     mPermX;
    std::vector<int>     mPermY;
    std::vector<int>     mPermZ;

    /**
     * @brief Perform Perlin interpolation.
     */
    static float interpolate(const Vector c[2][2][2], float u, float v, float w);

    /**
     * @brief Generate a set of permuatation ints.
     */
    static void generatePermutations(std::vector<int> &perms);
};
