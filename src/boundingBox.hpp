#pragma once

#include "ray.hpp"

class BoundingBox
{
  public:
    static constexpr float sPadding = 0.001; // Padding between the object and bounding box

    float mIntersections[3][2];

    BoundingBox();
    BoundingBox(float minX, float maxX, float minY, float maxY, float minZ, float maxZ);

    /**
     * @brief Returns true if a ray intersects with this
     * bounding box. Also returns the time (t) that the
     * ray hits the box, or infinity if no boxes were hit.
     */
    bool intersectsBox(const Ray &r, float &t);

    /**
     * @brief Merges another bounding box into this one.
     * Returns a reference to this.
     */
    BoundingBox &merge(const BoundingBox &other);

    /**
     * @brief Return the largest axis of this bounding box.
     */
    int largestAxis();

    /**
     * @brief Compare two bounding boxes along an axis. Return
     * true if a's min is less than b's min. False otherwise.
     */
    static bool compare(const BoundingBox &a, const BoundingBox &b, int axis);

  private:
    /**
     * @brief Calculate time in which the ray intersects a
     * particular point along a particular axis.
     */
    float intersectionTime(const Ray &r, float val, int axis);
};
