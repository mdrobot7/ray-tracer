#include "boundingBox.hpp"
#include "common.hpp"

#include <cmath>
#include <limits>

BoundingBox::BoundingBox()
{
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            mIntersections[i][j] = 0;
        }
    }
}

BoundingBox::BoundingBox(float minX, float maxX, float minY, float maxY, float minZ, float maxZ)
{
    mIntersections[V_X][0] = minX - sPadding;
    mIntersections[V_X][1] = maxX + sPadding;
    mIntersections[V_Y][0] = minY - sPadding;
    mIntersections[V_Y][1] = maxY + sPadding;
    mIntersections[V_Z][0] = minZ - sPadding;
    mIntersections[V_Z][1] = maxZ + sPadding;
}

bool BoundingBox::intersectsBox(const Ray &r, float &t)
{
    float minMaxInt = std::numeric_limits<float>::infinity();
    float maxMinInt = -std::numeric_limits<float>::infinity();

    // Find the intersection time range for each axis and track the
    // max of the minInts and min of the maxInts.
    for (int i = 0; i < 3; i++)
    {
        float int0       = intersectionTime(r, mIntersections[i][0], i);
        float int1       = intersectionTime(r, mIntersections[i][1], i);
        float thisMinInt = MIN(int0, int1);
        float thisMaxInt = MAX(int0, int1);
        maxMinInt        = thisMinInt > maxMinInt ? thisMinInt : maxMinInt;
        minMaxInt        = thisMaxInt < minMaxInt ? thisMaxInt : minMaxInt;
    }

    // Check if the ranges overlap, also ignore boxes that are *fully*
    // behind the ray
    if (maxMinInt < minMaxInt && minMaxInt > 0)
    {
        t = maxMinInt;
        return true;
    }
    t = std::numeric_limits<float>::infinity();
    return false;
}

float BoundingBox::intersectionTime(const Ray &r, float val, int axis)
{
    // Solve P = O + td for t
    return (val - r.mOrigin[axis]) / r.mDir[axis];
}

BoundingBox &BoundingBox::merge(const BoundingBox &other)
{
    for (int i = 0; i < 3; i++)
    {
        // Make as large a box as needed to hold both boxes
        if (mIntersections[i][0] > other.mIntersections[i][0])
        {
            mIntersections[i][0] = other.mIntersections[i][0];
        }
        if (mIntersections[i][1] < other.mIntersections[i][1])
        {
            mIntersections[i][1] = other.mIntersections[i][1];
        }
    }
    return *this;
}

int BoundingBox::largestAxis()
{
    float x = mIntersections[V_X][1] - mIntersections[V_X][0];
    float y = mIntersections[V_Y][1] - mIntersections[V_Y][0];
    float z = mIntersections[V_Z][1] - mIntersections[V_Z][0];
    if (x > y && x > z)
    {
        return V_X;
    }
    if (y > z)
    {
        return V_Y;
    }
    return V_Z;
}

bool BoundingBox::compare(const BoundingBox &a, const BoundingBox &b, int axis)
{
    return a.mIntersections[axis][0] < b.mIntersections[axis][0];
}
