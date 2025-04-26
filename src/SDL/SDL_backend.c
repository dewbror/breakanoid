/*
  SDL_backend.c
*/

#include "types.h"
#include "vulkan/vulkan_engine.h"
#include "SDL/SDL_backend.h"
#include "util/deletion_queue.h"

/**
 * SDL_DestroyWindow wrapper for use in the deletion queue.
 */
static void SDL_DestroyWindow_wrapper(void *p_SDL_window);

/**
 * SDL_Quit wrapper for use in the deletion queue.
 */
static void SDL_Quit_wrapper(void *p_not_used);

bool init_SDL_backend(vulkan_engine *p_engine) {
    // Initialize SDL
    if(!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        printf("Failed to initialize SDL library. Error: %s", SDL_GetError());
        return false;
    }
    deletion_queue_queue(p_engine->p_main_delq, NULL, SDL_Quit_wrapper);

    // Create SDL window
    SDL_WindowFlags windowFlags =
        SDL_WINDOW_VULKAN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;
    p_engine->p_SDL_window = SDL_CreateWindow("temp", (int)p_engine->window_extent.width, (int)p_engine->window_extent.height, windowFlags);
    if(!p_engine->p_SDL_window) {
        printf("Failed to create SDL window. Error: %s", SDL_GetError());
        return false;
    }
    deletion_queue_queue(p_engine->p_main_delq, p_engine->p_SDL_window, SDL_DestroyWindow_wrapper);
    printf("SDL initialized\n");
    return true;
}

static void SDL_Quit_wrapper(void *p_not_used) {
    printf("Callback: SDL_Quit_wrapper\n");
    // Cleanup all initialized SDL subsystems
    SDL_Quit();
}

static void SDL_DestroyWindow_wrapper(void *p_SDL_window) {
    printf("Callback: SDL_DestroyWindow_wrapper\n");
    // Destroy SDL window
    SDL_DestroyWindow((SDL_Window*)p_SDL_window);
}
