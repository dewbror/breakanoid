/**
 * Author: William Brorsson
 * Date Created: April 17, 2025
 *
 * More info to come...
 *
 * main.c
 */

#ifdef SYS_LINUX
// #include <X11/Xlib.h> // <-- probably not needed.
// https://registry.khronos.org/vulkan/specs/latest/man/html/WSIheaders.html
#else
#ifdef SYS_WINNT
#include <windows.h> // <-- needs to be included before std headers?
#endif
#endif

// STB headers. Move to vulkan_engine.c?
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// My headers
#include "types.h"
#include "version.h"
#include "vulkan/vulkan_engine.h"

static bool _main();

#ifdef SYS_WINNT
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    if(!_main())
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
#else
#ifdef SYS_LINUX
int main(int, char **) {
    if(!_main())
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
#endif
#endif

static bool _main() {
#ifndef NDEBUG
    printf("THIS IS A DEBUG BUILD!\n");
#endif
    printf("Version %s+%s.%s\n", breakanoid_VERSION, GIT_BRANCH, GIT_COMMIT_HASH);

    // Declare and zero initialize vulkan engine. Should i be using memset?
    vulkan_engine engine = {};
    // memset(&engine, 0, sizeof(vulkan_engine));
    bool success = vulkan_engine_init(&engine);
    if(success) {
        printf("Vulkan engine initialized\n");
        // game;
        // game_run(game, &engine);
        // game_quit(game);
    }
    // Destroy vulkan engine
    vulkan_engine_destroy(&engine);
    return success;
}
