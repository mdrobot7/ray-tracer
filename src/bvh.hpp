#pragma once

#include "boundingBox.hpp"
#include "color.hpp"
#include "ray.hpp"
#include "scene.hpp"
#include <vector>

class BoundingVolumeHierarchy
{
  public:
    // Intersection time must be larger than this, reduces shadow acne
    static constexpr float sMinT = 0.001;

    object::Primitive *mPrimitive;
    BoundingBox        mBbox;

    BoundingVolumeHierarchy();
    BoundingVolumeHierarchy(std::vector<std::unique_ptr<object::Primitive>> &primitives);
    BoundingVolumeHierarchy(std::vector<std::unique_ptr<object::Primitive>> &primitives,
                            size_t start, size_t end);
    ~BoundingVolumeHierarchy();

    /**
     * @brief Checks if a ray hits a bounding volume. If so,
     * return the Primitive it contains, otherwise NULL.
     *
     * If the BVH is traversed successfully this should return the
     * primitive that was hit.
     *
     * If any level of the BVH results in misses for both the
     * left and right child the method returns NULL.
     */
    object::Primitive::Collision intersects(const Ray &incoming, Ray &outgoing, float &t,
                                            Color &color);

  private:
    BoundingVolumeHierarchy *mLeft;
    BoundingVolumeHierarchy *mRight;

    void destroySubtree(BoundingVolumeHierarchy *subtree);

    static bool compare_x(const std::unique_ptr<object::Primitive> &a,
                          const std::unique_ptr<object::Primitive> &b);
    static bool compare_y(const std::unique_ptr<object::Primitive> &a,
                          const std::unique_ptr<object::Primitive> &b);
    static bool compare_z(const std::unique_ptr<object::Primitive> &a,
                          const std::unique_ptr<object::Primitive> &b);
};
