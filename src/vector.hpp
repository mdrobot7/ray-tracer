#pragma once

#include "common.hpp"
#include "yaml-cpp/yaml.h"
#include <cmath>
#include <cstddef>
#include <ostream>
#include <stdexcept>

#define V_X (0)
#define V_Y (1)
#define V_Z (2)

class Vector
{
  public:
    float x, y, z;

    constexpr Vector() : x(0), y(0), z(0) {}
    ~Vector() = default;
    constexpr Vector(const Vector &copy_from) : x(copy_from.x), y(copy_from.y), z(copy_from.z) {}
    constexpr Vector(Vector &&move_from) : x(move_from.x), y(move_from.y), z(move_from.z) {}
    constexpr Vector(float float_arr[3]) : x(float_arr[0]), y(float_arr[1]), z(float_arr[2]) {}
    constexpr Vector(float x, float y, float z) : x(x), y(y), z(z) {}

    constexpr Vector &operator=(const Vector &copy_from)
    {
        x = copy_from.x;
        y = copy_from.y;
        z = copy_from.z;
        return *this;
    }

    constexpr float &operator[](size_t index)
    {
        switch (index)
        {
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
            default:
                throw std::invalid_argument("Index out of range.");
        }
    }

    constexpr float operator[](size_t index) const
    {
        switch (index)
        {
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
            default:
                throw std::invalid_argument("Index out of range.");
        }
    }

    friend std::ostream &operator<<(std::ostream &os, const Vector &v)
    {
        os << "[ " << v.x << ", " << v.y << ", " << v.z << " ]";
        return os;
    }

    /**
     * NOTE!
     * All methods with 1 argument operate on this and
     * the provided argument, modify THIS Vector,
     * and return a reference to this. All methods with
     * 2 arguments operate on those arguments, store
     * the result in THIS, and return a reference to this.
     */

    /**
     * @brief 3-dimensional dot product of two vectors
     */
    static constexpr float dot(const Vector &a, const Vector &b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    constexpr float dot(const Vector &a)
    {
        return dot(*this, a);
    }

    /**
     * @brief 3-dimensional cross product of two vectors.
     * Performs this x a.
     */
    constexpr Vector cross3(const Vector &a, const Vector &b)
    {
        x = a.y * b.z - a.z * b.y;
        y = -1.0 * (a.x * b.z - a.z * b.x);
        z = a.x * b.y - a.y * b.x;
        return *this;
    }

    constexpr Vector cross3(const Vector &a)
    {
        return cross3(*this, a);
    }

    static constexpr Vector scross3(const Vector &a, const Vector &b)
    {
        Vector v;
        return v.cross3(a, b);
    }

    /**
     * @brief 3-dimensional vector add
     */
    friend constexpr Vector operator+(const Vector &a, const Vector &b)
    {
        return Vector(a.x + b.x, a.y + b.y, a.z + b.z);
    }

    constexpr Vector &operator+=(const Vector &a)
    {
        x += a.x;
        y += a.y;
        z += a.z;
        return *this;
    }

    /**
     * @brief 3-dimensional vector subtract.
     * Performs this - a.
     */
    friend constexpr Vector operator-(const Vector &a, const Vector &b)
    {
        return Vector(a.x - b.x, a.y - b.y, a.z - b.z);
    }

    constexpr Vector &operator-=(const Vector &a)
    {
        x -= a.x;
        y -= a.y;
        z -= a.z;
        return *this;
    }

    /**
     * @brief 3-dimensional vector multiply with a scalar
     */
    friend constexpr Vector operator*(const Vector &a, const float scale)
    {
        return Vector(a.x * scale, a.y * scale, a.z * scale);
    }

    friend constexpr Vector operator*(const float scale, const Vector &a)
    {
        return Vector(a.x * scale, a.y * scale, a.z * scale);
    }

    constexpr Vector &operator*=(const float scale)
    {
        x *= scale;
        y *= scale;
        z *= scale;
        return *this;
    }

    constexpr Vector operator-()
    {
        return *this * -1.0;
    }

    /**
     * @brief 3-dimensional vector length.
     */
    constexpr float length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    /**
     * @brief 3-dimensional vector norm. Attempting to normalize
     * the 0 vector returns the 0 vector.
     */
    constexpr Vector &norm()
    {
        if (this->length() == 0.0)
        {
            x = y = z = 0;
        }
        else
        {
            float len  = length();
            x         /= len;
            y         /= len;
            z         /= len;
        }
        return *this;
    }

    static constexpr Vector snorm(const Vector &a)
    {
        if (a.length() == 0.0)
        {
            return Vector(0, 0, 0);
        }
        else
        {
            return a * (1.0 / a.length());
        }
    }

    /**
     * @brief Clamp all elements of a vector in the range [0, clip]
     * (inclusive).
     */
    constexpr Vector &clip(float max)
    {
        x = CLAMP(x, 0, max);
        y = CLAMP(y, 0, max);
        z = CLAMP(z, 0, max);
        return *this;
    }

    /**
     * @brief Return a random normalized 3-dimensional vector.
     * Uses thread-safe C++ random number generation.
     */
    static inline Vector rand()
    {
        Vector v;
        v.x = randDist(randGen);
        v.y = randDist(randGen);
        v.z = randDist(randGen);
        return v.norm();
    }

    /**
     * @brief Returns true if all three of a vector's dimensions
     * are close to 0. False otherwise.
     */
    constexpr bool closeToZero()
    {
        return CLOSE_TO(x, 0.0) && CLOSE_TO(y, 0.0) && CLOSE_TO(z, 0.0);
    }
};

class Matrix
{
  public:
    float mMatrix[4][4]; // [row][col]

    constexpr Matrix() : mMatrix{} {}

    // rotation vector is in *degrees* around the x, y, and z axes
    constexpr Matrix(const Vector &origin, const Vector &rotation, const Vector &scale) : mMatrix{}
    {
        const float pi_180 = M_PI / 180.0;
        const float sin_x  = sin(rotation[V_X] * pi_180);
        const float cos_x  = cos(rotation[V_X] * pi_180);
        const float sin_y  = sin(rotation[V_Y] * pi_180);
        const float cos_y  = cos(rotation[V_Y] * pi_180);
        const float sin_z  = sin(rotation[V_Z] * pi_180);
        const float cos_z  = cos(rotation[V_Z] * pi_180);
        float       rotationMat[4][4]{};
        float       scaleMat[4][4]{};
        float       translationMat[4][4]{};
        float       translationScaleMat[4][4]{};

        // Assemble rotation, scale, and translation matrices
        // Rotation is X, then Y, then Z
        rotationMat[0][0] = cos_y * cos_z;
        rotationMat[0][1] = sin_x * sin_y * cos_z - cos_x * sin_z;
        rotationMat[0][2] = cos_x * sin_y * cos_z + sin_x * sin_z;

        rotationMat[1][0] = cos_y * sin_z;
        rotationMat[1][1] = sin_x * sin_y * sin_z + cos_x * cos_z;
        rotationMat[1][2] = cos_x * sin_y * sin_z - sin_x * cos_z;

        rotationMat[2][0] = -sin_y;
        rotationMat[2][1] = sin_x * cos_y;
        rotationMat[2][2] = cos_x * cos_y;

        rotationMat[3][3] = 1.0;

        scaleMat[0][0] = scale[V_X];
        scaleMat[1][1] = scale[V_Y];
        scaleMat[2][2] = scale[V_Z];
        scaleMat[3][3] = 1.0;

        translationMat[0][3] = origin[V_X];
        translationMat[1][3] = origin[V_Y];
        translationMat[2][3] = origin[V_Z];
        translationMat[0][0] = 1.0;
        translationMat[1][1] = 1.0;
        translationMat[2][2] = 1.0;
        translationMat[3][3] = 1.0;

        // Multiply matrices in *reverse* order, transformations
        // apply in the order: rotate, scale, translate
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                for (int k = 0; k < 4; k++)
                {
                    translationScaleMat[i][j] += translationMat[i][k] * scaleMat[k][j];
                }
            }
        }
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                for (int k = 0; k < 4; k++)
                {
                    mMatrix[i][j] += translationScaleMat[i][k] * rotationMat[k][j];
                }
            }
        }
    }

    friend std::ostream &operator<<(std::ostream &os, const Matrix &m)
    {
        os << "[ " << m.mMatrix[0][0] << ", " << m.mMatrix[0][1] << ", " << m.mMatrix[0][2] << ", "
           << m.mMatrix[0][3] << " ]" << std::endl;
        os << "[ " << m.mMatrix[1][0] << ", " << m.mMatrix[1][1] << ", " << m.mMatrix[1][2] << ", "
           << m.mMatrix[1][3] << " ]" << std::endl;
        os << "[ " << m.mMatrix[2][0] << ", " << m.mMatrix[2][1] << ", " << m.mMatrix[2][2] << ", "
           << m.mMatrix[2][3] << " ]" << std::endl;
        os << "[ " << m.mMatrix[3][0] << ", " << m.mMatrix[3][1] << ", " << m.mMatrix[3][2] << ", "
           << m.mMatrix[3][3] << " ]" << std::endl;
        return os;
    }

    /**
     * @brief Turn a 3-vector into a homogeneous 4-vector,
     * multiply by the model matrix, convert back to a
     * 3-vector and return.
     *
     * Modifies vec3 and returns a reference to vec3.
     */
    constexpr Vector &mul(Vector &vec3) const
    {
        float       temp[4]        = {};
        const float homogeneous[4] = {vec3[V_X], vec3[V_Y], vec3[V_Z], 1.0};

        // Multiply M * v, with v being a column vector
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                temp[i] += mMatrix[i][j] * homogeneous[j];
            }
        }

        vec3[V_X] = temp[0] / temp[3];
        vec3[V_Y] = temp[1] / temp[3];
        vec3[V_Z] = temp[2] / temp[3];
        return vec3;
    }
};

// yaml-cpp YAML::Node <-> Vector converter
namespace YAML
{
template <> struct convert<Vector>
{
    static Node encode(const Vector &rhs)
    {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);
        node.push_back(rhs.z);
        return node;
    }

    static bool decode(const Node &node, Vector &rhs)
    {
        if (!node.IsSequence() || node.size() != 3)
        {
            return false;
        }

        rhs.x = node[V_X].as<float>();
        rhs.y = node[V_Y].as<float>();
        rhs.z = node[V_Z].as<float>();
        return true;
    }
};
} // namespace YAML
