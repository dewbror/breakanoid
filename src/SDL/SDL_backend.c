#include <stdio.h>
#include <stdbool.h>
#include <SDL3/SDL_init.h>

#include "logger.h"
#include "SDL/SDL_backend.h"

bool SDL_backend_init(SDL_Window** pp_window, int width, int height) {
    // Initialize SDL
    if(!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        // Handle SDL_InitSubSystem error
        LOG_ERROR("Failed to initialize SDL sub systems: %s", SDL_GetError());
        return false;
    }
    LOG_INFO("SDL sub system initialized");
    
    // Create SDL window
    SDL_WindowFlags windowFlags =
        SDL_WINDOW_VULKAN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;
    *pp_window = SDL_CreateWindow("temp", width, height, windowFlags);
    if(!*pp_window) {
        // Handle SDL_CreateWindow error
        LOG_ERROR("Failed to create SDL window: %s", SDL_GetError());
        return false;
    }
   
    LOG_INFO("SDL window created");
    LOG_INFO("SDL backend successfully initialized");
    return true;
}

void SDL_backend_destroy(void* p_void_window) {
    LOG_DEBUG("Callback: SDL_backend_destroy");

    if(p_void_window == NULL) {
        LOG_ERROR("SDL_backend_destroy: window is NULL");
        return;
    }

    // Cast pointer
    SDL_Window* p_window = (SDL_Window*)p_void_window;

    // Destroy window
    SDL_DestroyWindow(p_window);
    p_window = NULL;

    // Cleanup all initialized SDL subsystems
    SDL_Quit();
}
