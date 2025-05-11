/*
  Author: William Brorsson
  Date Created: April 17, 2025

  main.c
*/

// SDL_MAIN_IMPLEMENTATION
#include "SDL3/SDL_main.h"

// STB headers. Move to vulkan_engine.c?
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// My headers
#include "vulkan/vulkan_engine.h"
#include "game/game.h"
#include "version.h"
#include "logger.h"

int main(int argc, char** argv) {
    // UNUSED
    (void)argc;
    (void)argv;
    LOG_TRACE("Entering main()");
#ifndef NDEBUG
    LOG_INFO("This is a debug build");
#endif
    LOG_INFO("Build version: %s+%s.%s", breakanoid_VERSION, GIT_BRANCH, GIT_COMMIT_HASH);

    // Ive decided to not zero initialize the vulkan_engine, it takes time and i
    // want to write my code such that no field is used befor it should be used.
    vulkan_engine engine;
    // memset(&engine, 0, sizeof(vulkan_engine));
    bool success = vulkan_engine_init(&engine);
    if(success) {
        // Havent decided weather to zero initiate this struct or not.
        game_s game;
        game_run(&engine, &game);
        game_destroy(&game);
    }
    // Destroy vulkan engine
    vulkan_engine_destroy(&engine);
    return success;
}
