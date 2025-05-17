/*
  SDL_backend.c
*/

#include <stdio.h>

#include <SDL3/SDL_init.h>

#include "logger.h"
#include "vulkan/vulkan_engine.h"
#include "SDL/SDL_backend.h"
#include "util/deletion_queue.h"

/**
 * SDL_DestroyWindow wrapper for use in the deletion queue.
 */
static void SDL_DestroyWindow_wrapper(void* p_SDL_window);

/**
 * SDL_Quit wrapper for use in the deletion queue.
 */
static void SDL_Quit_wrapper(void* p_not_used);

bool init_SDL_backend(vulkan_engine_t* p_engine) {
    if(p_engine == NULL) {
        LOG_ERROR("init_SDL_backend: p_engine is NULL");
        return false;
    }

    // Initialize SDL
    if(!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        // Handle SDL_InitSubSystem error
        LOG_ERROR("Failed to initialize SDL sub systems: %s", SDL_GetError());
        return false;
    }
    LOG_INFO("SDL sub system initialized");
    // Add SDL_Quit() to the deletion queue
    if(!deletion_queue_queue(p_engine->p_main_delq, NULL, SDL_Quit_wrapper)) {
        // Handle deletion queue error
        LOG_ERROR("Failed to queue deletion node");
        SDL_Quit_wrapper(NULL);
        return false;
    }

    // Create SDL window
    SDL_WindowFlags windowFlags =
        SDL_WINDOW_VULKAN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;
    p_engine->p_SDL_window =
        SDL_CreateWindow("temp", (int)p_engine->window_extent.width, (int)p_engine->window_extent.height, windowFlags);
    if(!p_engine->p_SDL_window) {
        // Handle SDL_CreateWindow error
        LOG_ERROR("Failed to create SDL window: %s", SDL_GetError());
        return false;
    }
    if(!deletion_queue_queue(p_engine->p_main_delq, p_engine->p_SDL_window, SDL_DestroyWindow_wrapper)) {
        // Handle deletion queue error
        LOG_ERROR("Failed to queue deletion node");
        SDL_DestroyWindow_wrapper(p_engine->p_SDL_window);
        return false;
    }

    LOG_INFO("SDL window created");
    LOG_INFO("SDL backend successfully initialized")
    return true;
}

static void SDL_Quit_wrapper(void* p_not_used) {
    // UNUSED
    (void)p_not_used;

    LOG_DEBUG("Callback: SDL_Quit_wrapper");
    // Cleanup all initialized SDL subsystems
    SDL_Quit();
}

static void SDL_DestroyWindow_wrapper(void* p_SDL_window) {
    LOG_DEBUG("Callback: SDL_DestroyWindow_wrapper");
    // Destroy SDL window
    SDL_DestroyWindow((SDL_Window*)p_SDL_window);
}
