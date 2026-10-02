# COMS 336 Renderer

## Build and Run
Requires GCC (supporting C++11 or newer) and Make. Developed on Windows 11 using MSYS2, but presumably will work on Linux/Mac too.

```
make setup
make all
./build/render
```

### Software Requirements
- Software: `gcc, g++` supporting C++11 or newer, Make, CMake, Python 3.11+
- Documentation: MiKTeX or something that provides `pdflatex` and LaTeX packages.

### Makefile Targets
- `setup`: Sets up the project.
- `docs`: Compiles the documentation.
- `all`: Compiles and links the embedded software and runs stack analysis
- `compiledb`: Generates Clang-style `compile_commands.json` that improves VSCode's autocompletion using Python compiledb.
- `clean`: Clean the build environment.

## Third Party Libraries (/lib)
- yaml-cpp
  - Best yaml parser that supports anchors and references.
- TinyOBJLoader
  - RapidOBJ is a newer, faster version of TinyOBJLoader but it doesn't support the Windows/MSYS combo. It's not Windows or Linux so it doesn't know what to do
- stb, specifically `stb_image.h`
  - CImg is more powerful but it needs ImageMagick to load anything other than PPM files. I started there but had to switch over.

## Future Improvements
- Potential idea: Refactor json files into objects list and index list. Could allow more reusability

## Tasks
- [x] A camera with configurable position, orientation, and field of view
- [x] Anti-aliasing
- [x] Ray/sphere intersections
- [x] Ray/triangle intersections
- [x] The ability to load textures (file format(s) of your choice; may use third-party libraries)
      - Done through tinyobjloader
- [x] Textured spheres and triangles
      - Texture loading done through `stb_image.h`
- [x] The ability to load and render triangle meshes (file format(s) of your choice; may use third-party libraries for loading)
      - Done through tinyobjloader
- [x] A spatial subdivision acceleration structure of your choice
      - Done with bounding volume hierarchy
- [x] Specular, diffuse, and dielectric materials (per first volume of Ray Tracing in One Weekend series)
- [x] Emissive materials (lights)

Required extras, at least 25 pts (C-):
- [x] Object instancing: being able to copy/rescale/rotate the same object throughout the scene (10)
      - Done with scene JSON and preventing multiple loads of the same OBJ file.
- [x] Quads (10)
- [x] Parallelization (10)

Non required extras:
- [ ] High dynamic range images (10)
- [x] Volume rendering (smoke, clouds, etc.) (10)
- [x] Quadrics (15)
- [ ] Spectral rendering (30)
- [ ] BRDF materials (Bi-directional reflectance distribution functions) (30)
- [ ] Subsurface scattering (BSSRDFs) (30)
- [ ] Motion blur (10)
- [x] Defocus blur/depth of field (10)
- [x] Perlin noise (10)
- [ ] Cube maps (15)
- [ ] Importance sampling (15)
- [ ] Normal interpolation (smooth shading) (5)
- [ ] Hybrid rendering with a GPU (OpenGL/DirectX + ray tracing) (20)
- [ ] GPU acceleration (GPU computing w/ e.g., CUDA) (20)
- [ ] Adaptive sampling (15)
