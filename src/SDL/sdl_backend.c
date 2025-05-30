#include <SDL3/SDL_init.h>
#include <stdio.h>

#include "SDL/sdl_backend.h"

#include "error/error.h"
#include "error/sdl_error.h"
#include "logger.h"

error_t sdl_backend_init(SDL_Window** pp_window, const int width, const int height)
{
    // Initialize SDL
    if(!SDL_InitSubSystem(SDL_INIT_VIDEO))
        return error_init(ERR_SRC_SDL, SDL_ERR_INIT_SUB_SYSTEM, "%s: Failed to initialize SDL sub system: %s", __func__,
            SDL_GetError());

    LOG_INFO("SDL sub system initialized");

    // Create SDL window
    SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_HIGH_PIXEL_DENSITY |
        SDL_WINDOW_RESIZABLE;

    SDL_Window* p_window = SDL_CreateWindow("temp", width, height, windowFlags);
    if(p_window == NULL)
        return error_init(ERR_SRC_SDL, SDL_ERR_WINDOW, "%s: Failed to create SDL window: %s", __func__, SDL_GetError());

    LOG_INFO("SDL window created");

    LOG_INFO("SDL backend successfully initialized");

    *pp_window = p_window;

    return SUCCESS;
}

void sdl_backend_destroy(void* p_void_window)
{
    LOG_DEBUG("Callback: %s", __func__);

    if(p_void_window == NULL) {
        LOG_ERROR("%s: window is NULL", __func__);
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
