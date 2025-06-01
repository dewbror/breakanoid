#include <SDL3/SDL_init.h>
#include <stdio.h>

#include "SDL/sdl_backend.h"

#include "error/error.h"
#include "error/sdl_error.h"
#include "logger.h"
#include "util/deletion_stack.h"

/**
 * \brief Destroy SDL backend.
 *
 * \param[in] p_void_window Pointer to SDL window to be destroyed.
 */
static void sdl_backend_deinit(void* p_void_window);

error_t sdl_backend_init(deletion_stack_t* p_del_stack, const int width, const int height, SDL_Window** pp_window)
{
    // Initialize SDL
    if(!SDL_InitSubSystem(SDL_INIT_VIDEO))
        return error_init(ERR_SRC_SDL, SDL_ERR_INIT_SUB_SYSTEM, "%s: Failed to initialize SDL sub system: %s", __func__,
            SDL_GetError());

    LOG_INFO("SDL library initiated");

    // Create SDL window
    SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_HIGH_PIXEL_DENSITY |
        SDL_WINDOW_RESIZABLE;

    SDL_Window* p_window = SDL_CreateWindow("temp", width, height, windowFlags);
    if(p_window == NULL)
        return error_init(ERR_SRC_SDL, SDL_ERR_WINDOW, "%s: Failed to create SDL window: %s", __func__, SDL_GetError());

    LOG_INFO("SDL window created");

    // Add cleanup
    error_t err = deletion_stack_push(p_del_stack, p_window, sdl_backend_deinit);
    if(err.code != 0) {
        sdl_backend_deinit(p_window);
        return err;
    }

    *pp_window = p_window;

    LOG_DEBUG("%s: Successful", __func__);

    return SUCCESS;
}

static void sdl_backend_deinit(void* p_void_window)
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
