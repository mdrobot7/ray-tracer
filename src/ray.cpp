#include "ray.hpp"
#include "color.hpp"

Ray::Ray()
{
    mColor             = Color(1.0, 1.0, 1.0);
    mIndexOfRefraction = 1.0;
}

Ray::Ray(Vector origin, Vector dir)
{
    mOrigin            = origin;
    mDir               = dir;
    mColor             = Color(1.0, 1.0, 1.0);
    mIndexOfRefraction = 1.0; // Air
}

void Ray::addCollision(Color color)
{
    mColor = Color::attenuate(mColor, color);
}
