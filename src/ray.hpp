#pragma once
#include "color.hpp"
#include "vector.hpp"
#include <ostream>

class Ray
{
  public:
    Vector mOrigin;
    Vector mDir; // Normalized!

    Color mColor;

    float mIndexOfRefraction; // Index of refraction of the material we're currently in

    Ray();
    Ray(Vector origin, Vector dir);

    friend std::ostream &operator<<(std::ostream &os, const Ray &r)
    {
        os << "Orig: " << r.mOrigin << ", Dir: " << r.mDir << ", Color: " << r.mColor;
        return os;
    }

    void addCollision(Color color);
};
