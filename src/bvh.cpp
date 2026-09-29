#include "bvh.hpp"
#include "color.hpp"
#include "scene.hpp"

#include <algorithm>
#include <typeinfo>

BoundingVolumeHierarchy::BoundingVolumeHierarchy()
{
    mPrimitive = NULL;
    mLeft      = NULL;
    mRight     = NULL;
}

BoundingVolumeHierarchy::BoundingVolumeHierarchy(
    std::vector<std::unique_ptr<object::Primitive>> &primitives)
    : BoundingVolumeHierarchy(primitives, 0, primitives.size())
{
}

BoundingVolumeHierarchy::BoundingVolumeHierarchy(
    std::vector<std::unique_ptr<object::Primitive>> &primitives, size_t start, size_t end)
{
    // See ray tracing in one weekend, their implementation is pretty smart.
    // Just modifying it so it fits how I have the rest of my system set up.
    mPrimitive = NULL;
    mLeft      = NULL;
    mRight     = NULL;

    // Find the longest axis
    mBbox = BoundingBox();
    for (int i = start; i < end; i++)
    {
        mBbox.merge(primitives[i]->mBoundingBox);
    }
    int axis  = mBbox.largestAxis();
    int range = end - start;

    // Go down the tree, generating nodes and assigning bounding boxes.
    // Skip the camera, it doesn't have a bounding box (it's still a Primitive
    // for json reasons).
    if (range == 1 && typeid(*primitives[start]) != typeid(object::Camera))
    {
        mPrimitive = primitives[start].get();
        mBbox      = mPrimitive->mBoundingBox;
    }
    else if (range == 2)
    {
        if (typeid(*primitives[start]) != typeid(object::Camera))
        {
            mLeft             = new BoundingVolumeHierarchy();
            mLeft->mPrimitive = primitives[start].get();
            mLeft->mBbox      = mLeft->mPrimitive->mBoundingBox;
        }
        if (typeid(*primitives[start + 1]) != typeid(object::Camera))
        {
            mRight             = new BoundingVolumeHierarchy();
            mRight->mPrimitive = primitives[start + 1].get();
            mRight->mBbox      = mRight->mPrimitive->mBoundingBox;
        }
    }
    else
    {
        // Recurse into each half of the BVH, split by object number along
        // the longest axis.

        // Can't pass args to std::sort callback, so we're doing this
        auto comparator = (axis == V_X)   ? BoundingVolumeHierarchy::compare_x
                          : (axis == V_Y) ? BoundingVolumeHierarchy::compare_y
                                          : BoundingVolumeHierarchy::compare_z;

        std::sort(std::begin(primitives) + start, std::begin(primitives) + end, comparator);
        mLeft  = new BoundingVolumeHierarchy(primitives, start, start + range / 2);
        mRight = new BoundingVolumeHierarchy(primitives, start + range / 2, end);
    }
}

BoundingVolumeHierarchy::~BoundingVolumeHierarchy()
{
    destroySubtree(this);
}

object::Primitive::Collision BoundingVolumeHierarchy::intersects(const Ray &incoming, Ray &outgoing,
                                                                 float &t, Color &color)
{
    // Traverse down any nodes that intersect the ray
    // (regardless of what side of the tree they're on).
    // If a leaf node is reached run the intersection test
    // and record the results.
    //
    // As you traverse, ignore any nodes with a farther
    // intersection than the one you already have. If you
    // find one closer you have to go all the way down until
    // you get a miss on the bounding volume or you reach
    // a primitive.

    object::Primitive::Collision collision = object::Primitive::Collision::MISSED;
    if (mPrimitive)
    {
        // Reached a leaf
        Ray   thisIncoming = Ray(incoming);
        float thisT        = t;
        Color thisColor    = Color(color);
        collision          = mPrimitive->collide(thisIncoming, thisT, thisColor);
        if (collision != object::Primitive::Collision::MISSED && thisT < t && thisT > sMinT)
        {
            // We hit something closer than our current mark, so remember it
            outgoing = Ray(thisIncoming);
            t        = thisT;
            color    = Color(thisColor);
            return collision;
        }
        return object::Primitive::Collision::MISSED;
    }

    float tLeft, tRight;
    bool  intLeft  = mLeft->mBbox.intersectsBox(incoming, tLeft);
    bool  intRight = mRight->mBbox.intersectsBox(incoming, tRight);

    if (intLeft)
    {
        collision = mLeft->intersects(incoming, outgoing, t, color);
    }
    if (intRight)
    {
        Ray                          thisOutgoing = Ray();
        float                        thisT        = t;
        Color                        thisColor    = Color(color);
        object::Primitive::Collision rightCollision =
            mRight->intersects(incoming, thisOutgoing, thisT, thisColor);
        if (rightCollision != object::Primitive::Collision::MISSED && thisT < t)
        {
            outgoing  = Ray(thisOutgoing);
            t         = thisT;
            color     = Color(thisColor);
            collision = rightCollision;
        }
    }
    return collision;
}

void BoundingVolumeHierarchy::destroySubtree(BoundingVolumeHierarchy *subtree)
{
    if (subtree->mLeft)
    {
        destroySubtree(subtree->mLeft);
    }
    if (subtree->mRight)
    {
        destroySubtree(subtree->mRight);
    }
}

bool BoundingVolumeHierarchy::compare_x(const std::unique_ptr<object::Primitive> &a,
                                        const std::unique_ptr<object::Primitive> &b)
{
    return BoundingBox::compare(a->mBoundingBox, b->mBoundingBox, V_X);
}

bool BoundingVolumeHierarchy::compare_y(const std::unique_ptr<object::Primitive> &a,
                                        const std::unique_ptr<object::Primitive> &b)
{
    return BoundingBox::compare(a->mBoundingBox, b->mBoundingBox, V_Y);
}

bool BoundingVolumeHierarchy::compare_z(const std::unique_ptr<object::Primitive> &a,
                                        const std::unique_ptr<object::Primitive> &b)
{
    return BoundingBox::compare(a->mBoundingBox, b->mBoundingBox, V_Z);
}
