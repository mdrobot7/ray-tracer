#pragma once

#include "common.hpp"
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

class ModelMatrix
{
  public:
    Vector mOrigin;
    Vector mFront; // Model's +Z axis
    Vector mTop;   // Model's +Y axis
    Vector mRight; // Model's +X axis
    Vector mScale;

    constexpr ModelMatrix() {}
    constexpr ModelMatrix(const Vector &origin, const Vector &front, const Vector &top,
                          const Vector &scale)
        : mOrigin(origin), mFront(front), mTop(top), mRight(), mScale(scale)
    {
        mRight = Vector::scross3(-mFront, mTop);
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
        /*
            Essentially this amounts to a change of basis,
            translation, and scale. Doing it with discrete operations
            instead of a single homogeneous matrix because
            it's easier to write and performance is probably
            close enough.

            Little hack  for change of basis because top and
            front are guaranteed to be orthogonal:
            V_i = (V dot b_i) / |b_i|^2
        */
        Vector temp;

        // Handle rotation with change of basis
        temp.x = Vector::dot(vec3, mRight) / Vector::dot(mRight, mRight);
        temp.y = Vector::dot(vec3, mTop) / Vector::dot(mTop, mTop);
        temp.z = Vector::dot(vec3, mFront) / Vector::dot(mFront, mFront);

        // Handle scale
        temp.x *= mScale.x;
        temp.y *= mScale.y;
        temp.z *= mScale.z;

        // Handle translation
        temp.x += mOrigin.x;
        temp.y += mOrigin.y;
        temp.z += mOrigin.z;

        vec3 = temp;
        return vec3;
    }
};
