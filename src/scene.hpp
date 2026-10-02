#pragma once
#include "instance.hpp"
#include "perlin.hpp"
#include "primitive.hpp"
#include "stb.hpp"
#include "tiny_obj_loader.h"
#include "vector.hpp"
#include "yaml-cpp/yaml.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * aspectRatio x 1 "unit" image plane. Origin vector points to the center
 * of the image plane, front and top determine orientation. Focal length
 * sets how far the "pinhole" is behind the image plane.
 */
class Camera
{
  public:
    Vector mOrigin;
    Vector mFront;
    Vector mTop;
    float  mFocalLength;
    float  mLensDiskDiameter;

    Camera();
    Camera(const Vector &origin, const Vector &rotation, float focalLength, float emissiveGain);
    Camera(YAML::Node &node);
};

class Scene
{
  public:
    Camera mCamera;

    std::vector<std::unique_ptr<object::Instance>> mInstances;

    // List of scene objects. Must be unique_ptr otherwise polymorphism breaks
    std::vector<std::unique_ptr<object::Primitive>> mPrimitives;

    // List of OBJ files, containing their assets
    std::unordered_map<std::string, std::unique_ptr<tinyobj::ObjReader>> mObjReaderMap;

    // List of textures
    std::unordered_map<std::string, std::unique_ptr<STBImage>> mTextureMap;

    Perlin mPerlin;

    Scene();
    ~Scene();

    /**
     * @brief Load the scene data from a JSON file and
     * populate all of the objects at their correct coordinates.
     *
     * @param sceneJsonPath
     */
    void load(std::string sceneJsonPath);

  private:
};
