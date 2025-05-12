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

    LOG_DEBUG("Entering main()");

    open_log_file();

#ifndef NDEBUG
    LOG_INFO("This is a debug build");
#endif
    LOG_INFO("Build version: %s+%s.%s", breakanoid_VERSION, GIT_BRANCH, GIT_COMMIT_HASH);

    vulkan_engine engine;
    bool success = vulkan_engine_init(&engine);
    if(success) {
        // Start game
        game_s game;
        success = game_run(&engine, &game);
        game_destroy(&game);
    }

    // Destroy vulkan engine
    if(!vulkan_engine_destroy(&engine) || !success) {
        LOG_ERROR("Exiting with failure");
        close_log_file();
        return EXIT_FAILURE;
    }

    close_log_file();
    // if(!success) {
    //     LOG_ERROR("Exiting with failure");
    //     return EXIT_FAILURE;
    // }

    LOG_INFO("Exiting with success");
    return EXIT_SUCCESS;
}
