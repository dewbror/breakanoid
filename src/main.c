/*
  Author: William Brorsson <dewb@duck.com>
  Date Created: April 17, 2025

  main.c
*/

// SDL_MAIN_IMPLEMENTATION
#include <SDL3/SDL_main.h>

// STB headers. Move to vulkan_engine.c?
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// My headers
#include "error/error.h"
#include "version.h"
#include "logger.h"
#include "vulkan/vulkan_engine.h"
#include "game/game.h"

int main(int argc, char** argv) {
    // UNUSED
    (void)argc;
    (void)argv;

    logger_open(NULL);

    LOG_DEBUG("Entering main()");

#ifndef NDEBUG
    LOG_INFO("This is a debug build");
#endif

    LOG_INFO("Build version: %s+%s.%s", break_VERSION, GIT_BRANCH, GIT_COMMIT_HASH);

    int success = 0;

    vulkan_engine_t engine;
    error_t err = vulkan_engine_init(&engine);
    success += err.code;
    if(err.code != 0) {
        LOG_ERROR("Failed to initiate vulkan engine: %s", err.msg);
        error_destroy(&err);
    } else {
        // vulkan engine initiated successfully, start game
        game_t game;
        game_init(&engine, &game);
        game_destroy(&game);
    }

    // Destroy vulkan engine
    err = vulkan_engine_destroy(&engine);
    success += err.code;
    if(err.code != 0) {
        LOG_ERROR("Failed to destroy vulkan engine: %s", err.msg);
        error_destroy(&err);
    }

    // Close logger
    logger_close();

    // Exit program
    if(success != 0) {
        LOG_ERROR("Exiting with failure");
        return EXIT_FAILURE;
    } else {
        LOG_INFO("Exiting with success");
        return EXIT_SUCCESS;
    }
}
