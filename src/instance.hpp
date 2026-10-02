#pragma once
#include "boundingBox.hpp"
#include "color.hpp"
#include "primitive.hpp"
#include "ray.hpp"
#include "vector.hpp"
#include "yaml-cpp/yaml.h"

namespace object
{

/**
 * @class Instance
 * @brief An instance of a Primitive. Contains a reference to a Primitive
 * and a transformation matrix to place it in the scene.
 */
class Instance
{
  public:
    /**
     * @brief Construct an Instance
     *
     * @param node YAML instance node to parse
     * @param prim Primitive to instantiate
     */
    Instance(YAML::Node &node, Primitive &prim);

    /**
     * @brief Test for collision between a ray and this instance.
     *
     * @param incoming Incoming ray to test
     * @param t If the ray hits, the collision time is returned here
     * @param color Color of the ray after the collision
     * @return Type of collision
     */
    enum Primitive::Collision collide(Ray &incoming, float &t, Color &color) const;

    /**
     * @brief Get the bounding box of this instance
     *
     * @return A BoundingBox of the instance
     */
    BoundingBox boundingBox() const;

  private:
    Primitive  &mPrim;
    ModelMatrix mModelMatrix;
};

} // namespace object
