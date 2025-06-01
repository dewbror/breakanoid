#ifndef SDL_ERROR_H_
#define SDL_ERROR_H_

/**
 * Enum for the SDL error codes
 */
typedef enum {
    SDL_ERR_NONE = 0,
    SDL_ERR_BACKEND_INIT,
    SDL_ERR_INIT_SUB_SYSTEM,
    SDL_ERR_WINDOW,
    SDL_ERR_VULKAN_CREATE_SURFACE
} sdl_error_code_t;

#endif // SDL_ERROR_H_
