#pragma once
#include "boundingBox.hpp"
#include "color.hpp"
#include "perlin.hpp"
#include "stb.hpp"
#include "vector.hpp"
#include "yaml-cpp/yaml.h"

namespace object
{

class Primitive
{
  public:
    enum Collision
    {
        REFLECTED = 0,
        ABSORBED,
        MISSED,
    };

    static constexpr float sRefractionGlass = 1.458;
    static float sEmissiveGain; // Boost light brightness to a max of (sEmissiveGain * [1, 1, 1])

    enum Color::Surface mSurface;
    float               mIndexOfRefraction;
    Color               mColor;
    STBImage           *mTexture;
    BoundingBox         mBoundingBox;
    Perlin             *mPerlin;

    Primitive();
    Primitive(YAML::Node &node);

    virtual ~Primitive() {};

    /**
     * @brief Collide a ray with this object.
     *
     * @param incoming Incoming ray
     * @param t Time t of collision with the object. Always > 0.
     * @param color Color of the object at the collision point.
     * @return enum Collision Type of collision that occurred
     */
    virtual enum Collision collide(Ray &incoming, float &t, Color &color) const = 0;

    /**
     * @brief Return the bounding box for this primitive.
     *
     * @return BoundingBox
     */
    virtual BoundingBox boundingBox() const;

    /**
     * @brief Perform a texture lookup, returning a color.
     */
    void textureLookup(const Vector &intersection, float u, float v, Color &color) const;

    // Ray collision helpers (common to all object types)
    /**
     * @brief Calculate a ray's reflection based on the surface type,
     * set the final ray color, and return the reflection type.
     *
     * @param incoming Incoming ray to be bounced
     * @param intersection Intersection point
     * @param normal Normal vector of the surface at intersection point
     * @return enum Collision
     */
    enum Collision bounce(Ray &incoming, const Vector &intersection, const Vector &normal) const;

    /**
     * @brief Handle a specular reflection.
     *
     * @param incoming Incoming ray to be reflected
     * @param intersection Intersection point
     * @param normal Normal vector of collision surface
     * @return true If the ray should keep bouncing
     * @return false If the ray has been absorbed
     */
    bool specular(Ray &incoming, const Vector &intersection, const Vector &normal) const;

    /**
     * @brief Handle a diffuse reflection.
     *
     * @param incoming Incoming ray to be reflected
     * @param intersection Intersection point
     * @param normal Normal vector of collision surface
     * @return true If the ray should keep bouncing
     * @return false If the ray has been absorbed
     */
    bool diffuse(Ray &incoming, const Vector &intersection, const Vector &normal) const;

    /**
     * @brief Handle a dielectric reflection/refraction.
     *
     * @param incoming Incoming ray to be reflected.
     * @return true If the ray should keep bouncing
     * @return false If the ray has been absorbed
     */
    bool dielectric(Ray &incoming, const Vector &intersection, const Vector &normal,
                    float indexOfRefraction) const;
};

class Triangle : public Primitive
{
  public:
    Vector mVertices[3];
    Vector mTexcoords[3];
    Vector mNormal; // Normal vector, determined by winding order of vertices

    Triangle();
    Triangle(Vector vertices[3], Vector texcoords[3], enum Color::Surface surface,
             float indexOfRefraction, const Color &color);
    Triangle(YAML::Node &node);

    enum Collision collide(Ray &incoming, float &t, Color &color) const override;
    BoundingBox    boundingBox() const override;

  private:
    void textureLookup(float alpha, float beta, float gamma, const Vector &intersection,
                       Color &color) const;
};

class Sphere : public Primitive
{
  public:
    Vector mOrigin;
    float  mRadius;

    Sphere();
    Sphere(const Vector &origin, float radius, enum Color::Surface surface, float indexOfRefraction,
           const Color &color);
    Sphere(YAML::Node &node);

    virtual enum Collision collide(Ray &incoming, float &t, Color &color) const override;
    virtual BoundingBox    boundingBox() const override;

  private:
    void textureLookup(Vector &intersection, Color &color) const;
};

class SphereVolume : public Sphere
{
  public:
    float mNegInvDensity;

    SphereVolume(const Vector &origin, float radius, float density, Color &color);
    SphereVolume(YAML::Node &node);

    enum Collision collide(Ray &incoming, float &t, Color &color) const override;
    // No texture lookup support
    BoundingBox boundingBox() const override;
};

class Quadric : public Primitive
{
  public:
    Vector      mOrigin;
    float       mA2, mB2, mC2, mD2; // Squared parameters (can be negative)
    float       mMaxOnAxis;
    float       mMaxOffAxis;
    std::string mAxis; // Axis to bound

    Quadric();
    Quadric(const Vector &center, float a2, float b2, float c2, float d2, float maxOnAxis,
            float maxOffAxis, const std::string &axis, enum Color::Surface surface,
            float indexOfRefraction, const Color &color);
    Quadric(YAML::Node &node);

    virtual enum Collision collide(Ray &incoming, float &t, Color &color) const override;
    virtual BoundingBox    boundingBox() const override;

  private:
    void textureLookup(Vector &intersection, Color &color) const;
};

class Quad : public Primitive
{
  public:
    Vector mOrigin;
    Vector mWidth, mHeight;
    Vector mNormal;

    Quad();
    Quad(const Vector &origin, const Vector &width, const Vector &height,
         enum Color::Surface surface, float indexOfRefraction, const Color &color);
    Quad(YAML::Node &node);

    enum Collision collide(Ray &incoming, float &t, Color &color) const override;
    BoundingBox    boundingBox() const override;

  private:
    void   textureLookup(float alpha, float beta, const Vector &intersection, Color &color) const;
    Vector mW; // Used for intersection checking
};

class Model : public Primitive
{
  public:
    tinyobj::ObjReader &mObj;

    Model(tinyobj::ObjReader &obj, const Vector &origin, const Vector &front, const Vector &top,
          const Vector &scale, enum Color::Surface surface, float indexOfRefraction,
          const Color &color);
    Model(YAML::Node &node, tinyobj::ObjReader &obj);

    enum Collision collide(Ray &incoming, float &t, Color &color) const override;
    // No texture lookup support
    BoundingBox boundingBox() const override;
};

}; // namespace object
