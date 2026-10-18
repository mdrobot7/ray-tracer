#include "bvh.hpp"
#include "render.hpp"
#include "scene.hpp"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <unistd.h>

#define HELP                                                                                       \
    "COMS 336 Ray Tracing Renderer\n"                                                              \
    "usage: render.exe [options]\n\n"                                                              \
    "options:\n"                                                                                   \
    "-h                 Show this help message and exit\n"                                         \
    "-s [SCENE_YAML]    Scene file for the renderer.\n"                                            \
    "                       Default: scenes/sample.yaml\n"                                         \
    "-r [RESOLUTION]    Resolution for the renderer, in the form\n"                                \
    "                       [width]x[height]. Default: 1024x768\n"                                 \
    "-a [AA_LEVEL]      Anti-aliasing level (number of rays per pixel).\n"                         \
    "                       Default: 1\n"                                                          \
    "-d [DEPTH]         Max ray depth (number of bounces). Default: 50\n"                          \
    "-j [JOBS]          Job count. Default: 1\n"                                                   \
    "-o [OUTPUT]        Output file path. Outputs [OUTPUT].ppm (packed binary)\n"                  \
    "                       and [OUTPUT].txt.ppm (text). Default: render\n"

int main(int argc, char *argv[])
{
    int         opt;
    std::string scenePath = "scenes/sample.yaml";
    int         width = 1024, height = 768;
    int         antiAliasingLevel = 1;
    int         depth             = 50;
    int         jobs              = 1;
    std::string outputPath        = "render";
    while ((opt = getopt(argc, argv, "hs:r:a:d:j:o:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                std::cout << HELP << std::endl;
                return 0;
            case 's':
                scenePath = std::string(optarg);
                break;
            case 'r':
            {
                std::string optargStr(optarg);
                std::string widthStr  = optargStr.substr(0, optargStr.find('x'));
                std::string heightStr = optargStr.substr(optargStr.find('x') + 1);
                width                 = (int)std::stoul(widthStr);
                height                = (int)std::stoul(heightStr);
                break;
            }
            case 'a':
                antiAliasingLevel = (int)std::stoul(optarg);
                break;
            case 'd':
                depth = (int)std::stoul(optarg);
                break;
            case 'j':
                jobs = (int)std::stoul(optarg);
                break;
            case 'o':
                outputPath = std::string(optarg);
                break;
            default:
                return 1;
        }
    }

    try
    {
        std::time_t startTime = std::time(NULL);

        Scene s;
        std::cout << "Building scene..." << std::endl;
        s.load(scenePath);

        std::cout << "Generating bounding volumes..." << std::endl;
        BoundingVolumeHierarchy *bvh =
            new BoundingVolumeHierarchy(s.mPrimitives); // Must be heap alloc

        Render render(s, *bvh, width, height, antiAliasingLevel, jobs, depth);
        std::cout << "Launching renderer..." << std::endl;
        render.run();
        std::cout << "Saving output..." << std::endl;
        render.save(outputPath);
        delete bvh;

        char   timeElapsed[9];
        time_t interval = std::time(NULL) - startTime;
        strftime(timeElapsed, 9, "%T", gmtime(&interval));
        std::cout << "Done. Render completed in " << timeElapsed << "." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception " << e.what() << std::endl;
    }
    return 0;
}
