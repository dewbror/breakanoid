/*
  Author: William Brorsson
  Date Created: April 17, 2025

  main.c
*/

#include "SDL3/SDL_main.h"

// #ifdef __linux__
// // #include <X11/Xlib.h> // <-- probably not needed.
// // https://registry.khronos.org/vulkan/specs/latest/man/html/WSIheaders.html
// #include <unistd.h>
// #else
// #ifdef _WIN32
// #include <windows.h> // <-- needs to be included before std headers?
// #endif
// #endif

// STB headers. Move to vulkan_engine.c?
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// My headers
#include "types.h"
#include "version.h"
#include "vulkan/vulkan_engine.h"
#include "game/game.h"

// static bool my_main(void);

// #ifdef SYS_WINNT
// int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
//     // UNUSED
//     (void)hInstance;
//     (void)hPrevInstance;
//     (void)pCmdLine;
//     (void)nCmdShow;

//     if(!my_main())
//         return EXIT_FAILURE;
//     return EXIT_SUCCESS;
// }
// #else
// #ifdef SYS_LINUX
// int main(int argc, char **argv) {
//     // UNUSED
//     (void)argc;
//     (void)argv;

//     if(!my_main())
//         return EXIT_FAILURE;
//     return EXIT_SUCCESS;
// }
// #endif
// #endif

int main(int argc, char **argv) {
    // UNUSED
    (void)argc;
    (void)argv;
#ifndef NDEBUG
    printf("THIS IS A DEBUG BUILD!\n");
#endif
    printf("Version %s+%s.%s\n\n", breakanoid_VERSION, GIT_BRANCH, GIT_COMMIT_HASH);

    // Ive decided to not zero initialize the vulkan_engine, it takes time and i want to write my code such that no
    // field is used befor it should be used.
    vulkan_engine engine;
    // memset(&engine, 0, sizeof(vulkan_engine));
    bool success = vulkan_engine_init(&engine);
    if(success) {
        printf("Vulkan engine initialized\n");
        // Havent decided weather to zero initiate this struct or not.
        game_s game;
        game_run(&engine, &game);
        game_destroy(&game);
    }
    // Destroy vulkan engine
    vulkan_engine_destroy(&engine);
    return success;
}

