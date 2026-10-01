#include "scene.hpp"
#include "color.hpp"
#include "common.hpp"
#include "vector.hpp"
#include <fstream>
#include <stdexcept>
#include <string>

namespace object
{

float Primitive::sEmissiveGain = 1;

Primitive::Primitive() {}
Primitive::Primitive(nlohmann::json &json)
{
    (void)json;
}
BoundingBox Primitive::boundingBox() const
{
    return BoundingBox();
}

void Primitive::textureLookup(const Vector &intersection, float u, float v, Color &color) const
{
    color = mTexture ? mTexture->getUv(u, v) : mColor;
    if (mSurface == Color::Surface::EMISSIVE)
    {
        color *= sEmissiveGain;
    }
    if (mPerlin)
    {
        color *= mPerlin->get(intersection);
    }
}

enum Primitive::Collision Primitive::bounce(Ray &incoming, const Vector &intersection,
                                            const Vector &normal) const
{
    switch (mSurface)
    {
        case Color::SPECULAR:
            return (specular(incoming, intersection, normal) ? Collision::REFLECTED
                                                             : Collision::ABSORBED);
        case Color::DIFFUSE:
            return (diffuse(incoming, intersection, normal) ? Collision::REFLECTED
                                                            : Collision::ABSORBED);
        case Color::DIELECTRIC:
            return (dielectric(incoming, intersection, normal, mIndexOfRefraction)
                        ? Collision::REFLECTED
                        : Collision::ABSORBED);
        case Color::EMISSIVE:
            return Collision::ABSORBED; // Emissive surfaces never reflect
    }
    throw std::invalid_argument("Collision error");
}

bool Primitive::specular(Ray &incoming, const Vector &intersection, const Vector &normal) const
{
    static const float fuzziness = 0.0; // TODO: Potentially use later

    incoming.mOrigin = intersection;
    // mDir = mDir - normal * 2 * dot(mDir, normal)
    incoming.mDir = incoming.mDir - normal * (2 * Vector::dot(incoming.mDir, normal));
    incoming.mDir.norm();

    // Add fuzziness. If the fuzzy vector is shot inside the object,
    // the object just absorbs it and the ray stops.
    incoming.mDir += Vector::rand() * fuzziness;
    return (Vector::dot(incoming.mDir, normal) > 0.0);
}

bool Primitive::diffuse(Ray &incoming, const Vector &intersection, const Vector &normal) const
{
    // Reflect 1 ray with a Lambertian reflection
    incoming.mOrigin        = intersection;
    Vector scatterDirection = normal + Vector::rand();
    if (scatterDirection.closeToZero())
    {
        incoming.mDir = normal;
    }
    else
    {
        incoming.mDir = scatterDirection.norm();
    }
    return true;
}

bool Primitive::dielectric(Ray &incoming, const Vector &intersection, const Vector &normal,
                           float indexOfRefraction) const
{
    // Check whether we're coming in or out of an object.
    // Normal and incoming vector will be opposite each other.
    bool outsideObject = Vector::dot(incoming.mDir, normal) < 0.0;

    // Determine if we have total internal reflection
    float refractionIndex = outsideObject ? 1.0 / indexOfRefraction : indexOfRefraction;
    float cosTheta        = std::fmin(Vector::dot(-incoming.mDir, normal), 1.0);
    float sinTheta        = sqrt(1.0 - cosTheta * cosTheta);

    // Schlick's approximation
    float temp    = (1.0 - refractionIndex) / (1.0 + refractionIndex);
    temp          = temp * temp;
    float schlick = temp + (1.0 - temp) * pow(1.0 - cosTheta, 5.0);

    if ((refractionIndex * sinTheta > 1.0) || (schlick > randomDouble()))
    {
        // No solution for Snell's law, must reflect
        return specular(incoming, intersection, normal);
    }

    // See raytracing in one weekend. Complex math based on Snell's law.
    Vector rOutPerp     = (incoming.mDir + (normal * cosTheta)) * refractionIndex;
    Vector rOutParallel = normal * -sqrt(std::abs(1.0 - Vector::dot(rOutPerp, rOutPerp)));

    incoming.mDir    = (rOutPerp + rOutParallel).norm();
    incoming.mOrigin = intersection + 0.001 * incoming.mDir; // Make sure we're outside/inside

    return true;
}

Triangle::Triangle() {}

Triangle::Triangle(Vector vertices[3], Vector texcoords[3], enum Color::Surface surface,
                   float indexOfRefraction, const Color &color)
{
    mVertices[0]       = vertices[0];
    mVertices[1]       = vertices[1];
    mVertices[2]       = vertices[2];
    mTexcoords[0]      = texcoords[0];
    mTexcoords[1]      = texcoords[1];
    mTexcoords[2]      = texcoords[2];
    mSurface           = surface;
    mIndexOfRefraction = indexOfRefraction;
    mColor             = color;
    mTexture           = NULL;
    mPerlin            = NULL;

    // Assuming CCW winding order (standard for OBJ and OpenGL)
    mNormal.cross3(mVertices[1] - mVertices[0], mVertices[2] - mVertices[1]).norm();

    mBoundingBox = boundingBox();
}

Triangle::Triangle(nlohmann::json &json)
{
    for (int i = 0; i < 3; i++)
    {
        mVertices[i] =
            Vector(json["vertices"][i]["x"], json["vertices"][i]["y"], json["vertices"][i]["z"]);
        mTexcoords[i] = Vector(json["texcoords"][i]["u"], json["texcoords"][i]["v"], 0.0);
    }
    mTexture = NULL;
    mPerlin  = NULL;

    // Assuming CCW winding order (standard for OBJ and OpenGL)
    mNormal.cross3(mVertices[1] - mVertices[0], mVertices[2] - mVertices[1]).norm();
}

enum Primitive::Collision Triangle::collide(Ray &incoming, float &t, Color &color) const
{
    // Check ray-plane intersection
    float dirDotNorm = Vector::dot(incoming.mDir, mNormal);
    if (CLOSE_TO(dirDotNorm, 0.0))
    {
        // Incoming is parallel
        return Collision::MISSED;
    }

    t = (mVertices[0] - incoming.mOrigin).dot(mNormal) / dirDotNorm;
    if (t < 0)
    {
        // Don't hit things behind us
        return Collision::MISSED;
    }
    else if (CLOSE_TO(t, 0.0))
    {
        // Don't collide with an object we just collided with
        return Collision::MISSED;
    }

    Vector intersection = incoming.mOrigin + (incoming.mDir * t);

    Vector v0 = mVertices[1] - mVertices[0];
    Vector v1 = mVertices[2] - mVertices[0];
    Vector v2 = intersection - mVertices[0];

    float d00   = Vector::dot(v0, v0);
    float d01   = Vector::dot(v0, v1);
    float d11   = Vector::dot(v1, v1);
    float d20   = Vector::dot(v2, v0);
    float d21   = Vector::dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    float beta  = (d11 * d20 - d01 * d21) / denom;
    float gamma = (d00 * d21 - d01 * d20) / denom;
    float alpha = 1.0f - beta - gamma;

    if (alpha < 0.0 || beta < 0.0 || gamma < 0.0)
    {
        // Did not intersect
        return Collision::MISSED;
    }

    if (mSurface == Color::SPECULAR || mSurface == Color::DIELECTRIC)
    {
        color = Color(1, 1, 1);
    }
    else
    {
        textureLookup(alpha, beta, gamma, intersection, color);
    }

    // Bounce it
    return bounce(incoming, intersection, mNormal);
}

BoundingBox Triangle::boundingBox() const
{
    float minX = MIN(MIN(mVertices[0][V_X], mVertices[1][V_X]), mVertices[2][V_X]);
    float maxX = MAX(MAX(mVertices[0][V_X], mVertices[1][V_X]), mVertices[2][V_X]);
    float minY = MIN(MIN(mVertices[0][V_Y], mVertices[1][V_Y]), mVertices[2][V_Y]);
    float maxY = MAX(MAX(mVertices[0][V_Y], mVertices[1][V_Y]), mVertices[2][V_Y]);
    float minZ = MIN(MIN(mVertices[0][V_Z], mVertices[1][V_Z]), mVertices[2][V_Z]);
    float maxZ = MAX(MAX(mVertices[0][V_Z], mVertices[1][V_Z]), mVertices[2][V_Z]);
    return BoundingBox(minX, maxX, minY, maxY, minZ, maxZ);
}

void Triangle::textureLookup(float alpha, float beta, float gamma, const Vector &intersection,
                             Color &color) const
{
    // Thanks stack overflow
    // https://stackoverflow.com/questions/17164376/inferring-u-v-for-a-point-in-a-triangle-from-vertex-u-vs
    float u = alpha * mTexcoords[0][0] + beta * mTexcoords[1][0] + gamma * mTexcoords[2][0];
    float v = alpha * mTexcoords[0][1] + beta * mTexcoords[1][1] + gamma * mTexcoords[2][1];

    Primitive::textureLookup(intersection, u, v, color);
}

Quadric::Quadric() {}

Quadric::Quadric(const Vector &center, float a2, float b2, float c2, float d2, float maxOnAxis,
                 float maxOffAxis, const std::string &axis, enum Color::Surface surface,
                 float indexOfRefraction, const Color &color)
{
    mOrigin            = center;
    mA2                = a2;
    mB2                = b2;
    mC2                = c2;
    mD2                = d2;
    mMaxOnAxis         = maxOnAxis;
    mMaxOffAxis        = maxOffAxis;
    mAxis              = axis;
    mSurface           = surface;
    mIndexOfRefraction = indexOfRefraction;
    mColor             = color;
    mTexture           = NULL;
    mPerlin            = NULL;
    mBoundingBox       = boundingBox();
}

Quadric::Quadric(nlohmann::json json)
{
    mOrigin     = Vector(json["x"], json["y"], json["z"]);
    mA2         = json["a2"];
    mB2         = json["b2"];
    mC2         = json["c2"];
    mD2         = json["d2"];
    mMaxOnAxis  = json["maxOnAxis"];
    mMaxOffAxis = json["maxOffAxis"];
    mAxis       = json["axis"];
    if (mAxis != "x" && mAxis != "y" && mAxis != "z")
    {
        throw std::invalid_argument("Invalid axis");
    }
    mTexture = NULL;
    mPerlin  = NULL;
}

enum Primitive::Collision Quadric::collide(Ray &incoming, float &t, Color &color) const
{
    // a2, b2, c2, d2 are the squared and signed versions of a, b, c, d in the
    // hyperboloid equation. Reorganize (Cx - X)^2/a2 + (Cy - Y)^2/b2 + (Cz - Z)^2/c2 = d2
    // into a quadratic equation w.r.t. t, solve with quadratic equation.
    // Special cases:
    // - Sphere: a2 = b2 = c2 = 1, d2 = radius
    // - Cone: one of a2, b2, c2 is negative, d2 = 0
    // - Cylinder: one of a2, b2, c2 is -inf, d2 = 1

    Vector centerMinusIncoming = mOrigin - incoming.mOrigin;

    float a  = mB2 * mC2 * incoming.mDir.x * incoming.mDir.x;
    a       += mA2 * mC2 * incoming.mDir.y * incoming.mDir.y;
    a       += mA2 * mB2 * incoming.mDir.z * incoming.mDir.z;
    float b  = 2.0 * mB2 * mC2 * incoming.mDir.x * centerMinusIncoming.x;
    b       += 2.0 * mA2 * mC2 * incoming.mDir.y * centerMinusIncoming.y;
    b       += 2.0 * mA2 * mB2 * incoming.mDir.z * centerMinusIncoming.z;
    b        = -b;
    float c  = mB2 * mC2 * centerMinusIncoming.x * centerMinusIncoming.x;
    c       += mA2 * mC2 * centerMinusIncoming.y * centerMinusIncoming.y;
    c       += mA2 * mB2 * centerMinusIncoming.z * centerMinusIncoming.z;
    c       -= mA2 * mB2 * mC2 * mD2;

    float discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0)
    {
        // No real roots
        return Collision::MISSED;
    }

    // Take the negative of the +/-, we want the smaller t (closer point)
    t = (-b - sqrt(discriminant)) / (2.0 * a);
    if (mSurface == Color::Surface::DIELECTRIC && CLOSE_TO(std::abs(t), 0.0))
    {
        // Dielectrics change ray direction at the surface of
        // the object. That leads to a lot of false Collision::MISSEDs
        // because we're either really close or negative.
        // Grab the further t.
        t = (-b + sqrt(discriminant)) / (2.0 * a);
    }

    if (t < 0)
    {
        // Don't hit things behind us
        return Collision::MISSED;
    }
    else if (CLOSE_TO(t, 0.0))
    {
        // Don't collide with an object we just collided with
        return Collision::MISSED;
    }

    Vector intersection = incoming.mOrigin + (incoming.mDir * t);

    // Ignore the reflection of the quadric over the origin plane (plane normal to mAxis)
    if (mAxis == "x" && intersection[0] < mOrigin[0])
    {
        return Collision::MISSED;
    }
    if (mAxis == "y" && intersection[1] < mOrigin[1])
    {
        return Collision::MISSED;
    }
    if (mAxis == "z" && intersection[2] < mOrigin[2])
    {
        return Collision::MISSED;
    }

    // Calculate surface normal using the gradient at the intersection point
    // gradient = <dF/dx, dF/dy, dF/dz> for those who forgot (those are all partial derivatives).
    // Conveniently, all quadrics have x^2 terms so it's just a matter of multiplying in
    // the coeffients and handling power rule.
    Vector gradient(
        intersection[0] * 2.0 / mA2, intersection[1] * 2.0 / mB2, intersection[2] * 2.0 / mC2);
    Vector normal = gradient.norm();
    if (mSurface == Color::SPECULAR || mSurface == Color::DIELECTRIC)
    {
        color = Color(1, 1, 1);
    }
    else
    {
        textureLookup(intersection, color);
    }

    // Bounce it
    return bounce(incoming, intersection, normal);
}

BoundingBox Quadric::boundingBox() const
{
    // Make a (mMax)^3 box pointing in the direction of mAxis
    if (mAxis == "x")
    {
        return BoundingBox(mOrigin[0] + 0,
                           mOrigin[0] + mMaxOnAxis,
                           mOrigin[1] + -mMaxOffAxis,
                           mOrigin[1] + mMaxOffAxis,
                           mOrigin[2] + -mMaxOffAxis,
                           mOrigin[2] + mMaxOffAxis);
    }
    if (mAxis == "y")
    {
        return BoundingBox(mOrigin[0] + -mMaxOffAxis,
                           mOrigin[0] + mMaxOffAxis,
                           mOrigin[1] + 0,
                           mOrigin[1] + mMaxOnAxis,
                           mOrigin[2] + -mMaxOffAxis,
                           mOrigin[2] + mMaxOffAxis);
    }
    return BoundingBox(mOrigin[0] + -mMaxOffAxis,
                       mOrigin[0] + mMaxOffAxis,
                       mOrigin[1] + -mMaxOffAxis,
                       mOrigin[1] + mMaxOffAxis,
                       mOrigin[2] + 0,
                       mOrigin[2] + mMaxOnAxis);
}

void Quadric::textureLookup(Vector &intersection, Color &color) const
{
    // No texture support, but needs to be here for overrides
    color = mColor;
}

Sphere::Sphere() {}

Sphere::Sphere(const Vector &center, float radius, enum Color::Surface surface,
               float indexOfRefraction, const Color &color)
{
    mOrigin            = center;
    mRadius            = radius;
    mSurface           = surface;
    mIndexOfRefraction = indexOfRefraction;
    mColor             = color;
    mTexture           = NULL;
    mPerlin            = NULL;
    mBoundingBox       = boundingBox();
}

Sphere::Sphere(nlohmann::json &json)
{
    mOrigin  = Vector(json["x"], json["y"], json["z"]);
    mRadius  = json["radius"];
    mTexture = NULL;
    mPerlin  = NULL;
}

enum Primitive::Collision Sphere::collide(Ray &incoming, float &t, Color &color) const
{
    // The math for this is really complicated, it's basically
    // solving a quadratic equation. See Ray Tracing in One Weekend
    Vector centerMinusIncoming = mOrigin - incoming.mOrigin;
    float  a                   = Vector::dot(incoming.mDir, incoming.mDir);
    float  b                   = -2.0 * Vector::dot(incoming.mDir, centerMinusIncoming);
    float  c            = Vector::dot(centerMinusIncoming, centerMinusIncoming) - mRadius * mRadius;
    float  discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0)
    {
        // No intersection
        return Collision::MISSED;
    }
    else
    {
        // Take the negative of the +/-, we want the smaller t (closer point)
        t = (-b - sqrt(discriminant)) / (2.0 * a);
        if (mSurface == Color::Surface::DIELECTRIC && CLOSE_TO(std::abs(t), 0.0))
        {
            // Dielectrics change ray direction at the surface of
            // the object. That leads to a lot of false Collision::MISSEDs
            // because we're either really close or negative.
            // Grab the further t.
            t = (-b + sqrt(discriminant)) / (2.0 * a);
        }

        if (t < 0)
        {
            // Don't hit things behind us
            return Collision::MISSED;
        }
        else if (CLOSE_TO(t, 0.0))
        {
            // Don't collide with an object we just collided with
            return Collision::MISSED;
        }
    }

    Vector intersection = incoming.mOrigin + (incoming.mDir * t);
    Vector normal       = (intersection - mOrigin) * (1.0 / mRadius);
    if (mSurface == Color::SPECULAR || mSurface == Color::DIELECTRIC)
    {
        color = Color(1, 1, 1);
    }
    else
    {
        textureLookup(intersection, color);
    }

    // Bounce it
    return bounce(incoming, intersection, normal);
}

BoundingBox Sphere::boundingBox() const
{
    return BoundingBox(mOrigin[V_X] - mRadius,
                       mOrigin[V_X] + mRadius,
                       mOrigin[V_Y] - mRadius,
                       mOrigin[V_Y] + mRadius,
                       mOrigin[V_Z] - mRadius,
                       mOrigin[V_Z] + mRadius);
}

void Sphere::textureLookup(Vector &intersection, Color &color) const
{
    // Convert intersection point to spherical coordinates
    Vector unitSphereIntersection = (intersection - mOrigin) * (1.0 / mRadius);
    float  phi   = atan2(-unitSphereIntersection[V_Z], unitSphereIntersection[V_X]) + M_PI;
    float  theta = acos(-unitSphereIntersection[V_Y]);

    float u = phi / (2.0 * M_PI);
    float v = theta / M_PI;
    Primitive::textureLookup(intersection, u, v, color);
}

Quad::Quad() {}

Quad::Quad(const Vector &origin, const Vector &width, const Vector &height,
           enum Color::Surface surface, float indexOfRefraction, const Color &color)
{
    mOrigin            = origin;
    mWidth             = width;
    mHeight            = height;
    mSurface           = surface;
    mIndexOfRefraction = indexOfRefraction;
    mColor             = color;
    mTexture           = NULL;
    mPerlin            = NULL;

    Vector widthCrossHeight = Vector::scross3(mWidth, mHeight);
    mNormal                 = Vector::snorm(widthCrossHeight);
    mW = widthCrossHeight * (1.0 / Vector::dot(widthCrossHeight, widthCrossHeight));

    mBoundingBox = boundingBox();
}

Quad::Quad(nlohmann::json &json)
{
    mOrigin  = Vector(json["origin"]["x"], json["origin"]["y"], json["origin"]["z"]);
    mWidth   = Vector(json["width"]["x"], json["width"]["y"], json["width"]["z"]);
    mHeight  = Vector(json["height"]["x"], json["height"]["y"], json["height"]["z"]);
    mTexture = NULL;
    mPerlin  = NULL;

    Vector widthCrossHeight = Vector::scross3(mWidth, mHeight);
    mNormal                 = Vector::snorm(widthCrossHeight);
    mW = widthCrossHeight * (1.0 / Vector::dot(widthCrossHeight, widthCrossHeight));
}

enum Primitive::Collision Quad::collide(Ray &incoming, float &t, Color &color) const
{
    // Check ray-plane intersection
    float dirDotNorm = Vector::dot(incoming.mDir, mNormal);
    if (CLOSE_TO(dirDotNorm, 0.0))
    {
        // Incoming is parallel
        return Collision::MISSED;
    }

    t = Vector::dot(mOrigin - incoming.mOrigin, mNormal) / dirDotNorm;
    if (t < 0)
    {
        // Don't hit things behind us
        return Collision::MISSED;
    }
    else if (CLOSE_TO(t, 0.0))
    {
        // Don't collide with an object we just collided with
        return Collision::MISSED;
    }

    Vector intersection       = incoming.mOrigin + (incoming.mDir * t);
    Vector planarIntersection = intersection - mOrigin;
    float  alpha              = Vector::dot(mW, Vector::scross3(planarIntersection, mHeight));
    float  beta               = Vector::dot(mW, Vector::scross3(mWidth, planarIntersection));

    // planarIntersection = alpha * mWidth + beta * mHeight.
    // If alpha and beta are [0.0, 1.0], then the intersection is inside the quad.
    if (!IN_RANGE(alpha, 0.0, 1.0) || !IN_RANGE(beta, 0.0, 1.0))
    {
        return Collision::MISSED;
    }

    if (mSurface == Color::SPECULAR || mSurface == Color::DIELECTRIC)
    {
        color = Color(1, 1, 1);
    }
    else
    {
        textureLookup(alpha, beta, intersection, color);
    }

    // Bounce it
    return bounce(incoming, intersection, mNormal);
}

BoundingBox Quad::boundingBox() const
{
    Vector cornerW  = mOrigin + mWidth;
    Vector cornerH  = mOrigin + mHeight;
    Vector cornerWH = cornerW + mHeight;
    // 4-way min/max... this is ugly
    float minX = MIN(MIN(mOrigin[V_X], cornerW[V_X]), MIN(cornerH[V_X], cornerWH[V_X]));
    float maxX = MAX(MAX(mOrigin[V_X], cornerW[V_X]), MAX(cornerH[V_X], cornerWH[V_X]));
    float minY = MIN(MIN(mOrigin[V_Y], cornerW[V_Y]), MIN(cornerH[V_Y], cornerWH[V_Y]));
    float maxY = MAX(MAX(mOrigin[V_Y], cornerW[V_Y]), MAX(cornerH[V_Y], cornerWH[V_Y]));
    float minZ = MIN(MIN(mOrigin[V_Z], cornerW[V_Z]), MIN(cornerH[V_Z], cornerWH[V_Z]));
    float maxZ = MAX(MAX(mOrigin[V_Z], cornerW[V_Z]), MAX(cornerH[V_Z], cornerWH[V_Z]));
    return BoundingBox(minX, maxX, minY, maxY, minZ, maxZ);
}

void Quad::textureLookup(float alpha, float beta, const Vector &intersection, Color &color) const
{
    // Intersection testing gives us alpha and beta, which are
    // the same as u and v. mOrigin is at the top left of the image.
    Primitive::textureLookup(intersection, alpha, beta, color);
}

Model::Model(tinyobj::ObjReader &obj, const Vector &origin, const Vector &front, const Vector &top,
             const Vector &scale, enum Color::Surface surface, float indexOfRefraction,
             const Color &color)
    : mObj(obj)
{
    mModelMatrix       = ModelMatrix(origin, Vector::snorm(front), Vector::snorm(top), scale);
    mSurface           = surface;
    mIndexOfRefraction = indexOfRefraction;
    mColor             = color;
    mBoundingBox       = boundingBox();
}

Model::Model(nlohmann::json &json, tinyobj::ObjReader &obj) : mObj(obj)
{
    mModelMatrix =
        ModelMatrix(Vector(json["origin"]["x"], json["origin"]["y"], json["origin"]["z"]),
                    Vector(json["front"]["x"], json["front"]["y"], json["front"]["z"]).norm(),
                    Vector(json["top"]["x"], json["top"]["y"], json["top"]["z"]).norm(),
                    Vector(json["scale"]["x"], json["scale"]["y"], json["scale"]["z"]));
}

enum Primitive::Collision Model::collide(Ray &incoming, float &t, Color &color) const
{
    Triangle tri           = Triangle();
    tri.mSurface           = mSurface;
    tri.mIndexOfRefraction = mIndexOfRefraction;
    tri.mColor             = mColor;
    tri.mTexture           = NULL;
    tri.mPerlin            = NULL;

    Ray closestRay;
    t                          = std::numeric_limits<float>::infinity();
    Collision closestCollision = Collision::MISSED;
    Ray       thisRay          = Ray(incoming);
    float     thisT;
    Color     thisColor;
    Collision thisCollision;

    // Essentially rewrite the main render loop but for only this model
    auto &attrib      = mObj.GetAttrib();
    auto &shapes      = mObj.GetShapes();
    auto &materials   = mObj.GetMaterials();
    int   indexOffset = 0;
    for (size_t s = 0; s < shapes.size(); s++)
    {
        const tinyobj::shape_t &shape        = shapes[s];
        size_t                  numTriangles = shape.mesh.num_face_vertices.size();
        for (size_t n = 0; n < numTriangles; n++)
        {
            // Every face is going to be 3 vertices (almost always)
            // Fill in our triangle
            for (int i = 0; i < 3; i++)
            {
                // Index buffer lookup
                tinyobj::index_t index = shape.mesh.indices[indexOffset + i];
                // Vertex buffer lookup
                for (int j = 0; j < 3; j++)
                {
                    tri.mVertices[i][j] = attrib.vertices[3 * size_t(index.vertex_index) + j];
                }

                // Handle scaling, rotation, and positioning (model matrix).
                // Do on the fly so we can do proper object instancing (vertex shader-style)
                mModelMatrix.mul(tri.mVertices[i]);
            }
            // Fill in surface normal assuming CCW winding order (standard for OBJ and OpenGL)
            tri.mNormal.cross3(tri.mVertices[1] - tri.mVertices[0],
                               tri.mVertices[2] - tri.mVertices[1]);
            tri.mNormal.norm();

            thisCollision = tri.collide(thisRay, thisT, thisColor);
            if (thisCollision != Collision::MISSED && thisT < t)
            {
                // Found a closer collision
                closestRay       = Ray(thisRay);
                t                = thisT;
                color            = Color(thisColor);
                closestCollision = thisCollision;
            }
            indexOffset += 3;
        }
    }

    incoming = Ray(closestRay);
    // t, color set during execution
    return closestCollision;
}

BoundingBox Model::boundingBox() const
{
    // Slow... there's no faster way, you have to check every vertex
    float minX = std::numeric_limits<float>::infinity();
    float maxX = -std::numeric_limits<float>::infinity();
    float minY = std::numeric_limits<float>::infinity();
    float maxY = -std::numeric_limits<float>::infinity();
    float minZ = std::numeric_limits<float>::infinity();
    float maxZ = -std::numeric_limits<float>::infinity();

    auto &attrib      = mObj.GetAttrib();
    auto &shapes      = mObj.GetShapes();
    auto &materials   = mObj.GetMaterials();
    int   indexOffset = 0;
    for (size_t s = 0; s < shapes.size(); s++)
    {
        const tinyobj::shape_t &shape        = shapes[s];
        size_t                  numTriangles = shape.mesh.num_face_vertices.size();
        for (size_t n = 0; n < numTriangles; n++)
        {
            for (int i = 0; i < 3; i++)
            {
                // Index buffer lookup
                tinyobj::index_t index = shape.mesh.indices[indexOffset + i];
                Vector           v;
                // Vertex buffer lookup
                for (int j = 0; j < 3; j++)
                {
                    v[j] = attrib.vertices[3 * size_t(index.vertex_index) + j];
                }

                // Handle scaling, rotation, and positioning (model matrix).
                mModelMatrix.mul(v);

                if (v[V_X] < minX)
                    minX = v[V_X];
                if (v[V_X] > maxX)
                    maxX = v[V_X];
                if (v[V_Y] < minY)
                    minY = v[V_Y];
                if (v[V_Y] > maxY)
                    maxY = v[V_Y];
                if (v[V_Z] < minZ)
                    minZ = v[V_Z];
                if (v[V_Z] > maxZ)
                    maxZ = v[V_Z];
            }
            indexOffset += 3;
        }
    }
    return BoundingBox(minX, maxX, minY, maxY, minZ, maxZ);
}

SphereVolume::SphereVolume(const Vector &origin, float radius, float density, Color &color)
    : Sphere(origin, radius, Color::Surface::DIFFUSE, 0, color)
{
    mNegInvDensity = -1.0 / density;
}

SphereVolume::SphereVolume(nlohmann::json &json) : Sphere(json)
{
    mNegInvDensity = -1.0 / (float)(json["density"]);
}

enum Primitive::Collision SphereVolume::collide(Ray &incoming, float &t, Color &color) const
{
    // Find the length of time the ray spends inside of the volume.
    // Stolen from the sphere method -- can't fully reuse, it needs some modifications
    Vector centerMinusIncoming = mOrigin - incoming.mOrigin;
    float  a                   = Vector::dot(incoming.mDir, incoming.mDir);
    float  b                   = -2.0 * Vector::dot(incoming.mDir, centerMinusIncoming);
    float  c            = Vector::dot(centerMinusIncoming, centerMinusIncoming) - mRadius * mRadius;
    float  discriminant = b * b - 4.0 * a * c;
    float  minT, maxT;
    if (discriminant < 0)
    {
        return Collision::MISSED;
    }
    else
    {
        t = minT = (-b - sqrt(discriminant)) / (2.0 * a);
        maxT     = (-b + sqrt(discriminant)) / (2.0 * a);

        if (t < 0)
        {
            return Collision::MISSED;
        }
        else if (CLOSE_TO(t, 0.0))
        {
            return Collision::MISSED;
        }
    }
    float timeInVolume = std::abs(maxT - minT);

    // Check against random number, see ray tracing in one weekend
    float hitTime = mNegInvDensity * log(randomDouble());
    if (hitTime > timeInVolume)
    {
        return Collision::MISSED;
    }

    Vector intersection = incoming.mOrigin + (incoming.mDir * hitTime);

    if (mSurface == Color::SPECULAR || mSurface == Color::DIELECTRIC)
    {
        color = Color(1, 1, 1);
    }
    else
    {
        color = mColor;
    }

    // Bounce it, specular/emissive clouds could be funky
    return bounce(incoming, intersection, Vector::rand());
}

BoundingBox SphereVolume::boundingBox() const
{
    return Sphere::boundingBox();
}

Camera::Camera() {}

Camera::Camera(const Vector &origin, const Vector &front, const Vector &top, float focalLength,
               float emissiveGain)
{
    mOrigin                  = origin;
    mFront                   = Vector::snorm(front);
    mTop                     = Vector::snorm(top);
    mFocalLength             = focalLength;
    Primitive::sEmissiveGain = emissiveGain;
}

Camera::Camera(nlohmann::json &json)
{
    mOrigin           = Vector(json["origin"]["x"], json["origin"]["y"], json["origin"]["z"]);
    mFront            = Vector(json["front"]["x"], json["front"]["y"], json["front"]["z"]).norm();
    mTop              = Vector(json["top"]["x"], json["top"]["y"], json["top"]["z"]).norm();
    mFocalLength      = json["focalLength"];
    mLensDiskDiameter = tan((float)(json["defocusAngle"]) * (M_PI / 180.0)) *
                        mFocalLength; // tan(angle) = opp / adj
    Primitive::sEmissiveGain = json["emissiveGain"];
}
} // namespace object

Scene::Scene() {}

Scene::~Scene()
{
    for (const auto &i : mTextures)
    {
        i->free();
    }
}

void Scene::load(std::string sceneJsonPath)
{
    std::ifstream f(sceneJsonPath);

    using json = nlohmann::json;
    json data  = json::parse(f);

    tinyobj::ObjReaderConfig readerConfig;
    readerConfig.mtl_search_path = "./assets/materials"; // Hardcoded, fight me

    mPerlin = Perlin();

    // Look at scenes/sample.json for the format
    mCamera = object::Camera(data["camera"]);
    for (json i : data["objects"])
    {
        if (i["type"] == "obj")
        {
            // Attributes and vertices are stored in the tinyobj::ObjReader object.
            // We could spend time deep-copying them out, but that seems foolish.
            // Just keep the reader around.

            // Don't load the same thing multiple times
            size_t fileIndex;
            for (fileIndex = 0; fileIndex < mObjFilenames.size(); fileIndex++)
            {
                if (mObjFilenames[fileIndex] == i["path"])
                {
                    break;
                }
            }
            if (fileIndex != mObjFilenames.size())
            {
                mPrimitives.push_back(std::make_unique<object::Model>(i, mObjReaders[fileIndex]));
            }
            else
            {
                mObjReaders.push_back(tinyobj::ObjReader());
                mObjFilenames.push_back(i["path"]);
                if (!mObjReaders.back().ParseFromFile(i["path"], readerConfig))
                {
                    f.close();
                    throw std::invalid_argument("OBJ file parse failed");
                }
                mPrimitives.push_back(std::make_unique<object::Model>(i, mObjReaders.back()));
            }
        }
        else if (i["type"] == "sphere")
        {
            mPrimitives.push_back(std::make_unique<object::Sphere>(i));
        }
        else if (i["type"] == "quadric")
        {
            mPrimitives.push_back(std::make_unique<object::Quadric>(i));
        }
        else if (i["type"] == "triangle")
        {
            mPrimitives.push_back(std::make_unique<object::Triangle>(i));
        }
        else if (i["type"] == "quad")
        {
            mPrimitives.push_back(std::make_unique<object::Quad>(i));
        }
        else if (i["type"] == "sphereVolume")
        {
            mPrimitives.push_back(std::make_unique<object::SphereVolume>(i));
        }
        else
        {
            f.close();
            throw std::invalid_argument("Invalid object in JSON");
        }

        // Fill in common attributes
        auto &p         = mPrimitives.back();
        p->mSurface     = Color::stringToSurface(i["surface"]);
        p->mBoundingBox = p->boundingBox();
        if (p->mSurface == Color::Surface::DIELECTRIC)
        {
            p->mIndexOfRefraction = i["indexOfRefraction"];
        }
        if (i["perlin"])
        {
            p->mPerlin = &mPerlin;
        }
        try
        {
            int color = std::stoi((std::string)(i["texture"]), 0, 16);
            p->mColor = Color::intToColor(color);
        }
        catch (std::invalid_argument const &)
        {
            if (i["type"] == "sphereVolume")
            {
                throw std::invalid_argument("Can't assign textures to volumes.");
            }

            // Texture is a path instead of a color
            size_t fileIndex;
            for (fileIndex = 0; fileIndex < mTextureFilenames.size(); fileIndex++)
            {
                if (mTextureFilenames[fileIndex] == i["texture"])
                {
                    break;
                }
            }
            if (fileIndex == mTextureFilenames.size())
            {
                mTextures.push_back(std::make_unique<STBImage>(i["texture"]));
                mTextureFilenames.push_back(i["texture"]);
            }
            p->mTexture = mTextures[fileIndex].get();
        }
    }

    f.close();

    if (mPrimitives.size() == 0)
    {
        throw std::invalid_argument("No objects in the scene");
    }
}
