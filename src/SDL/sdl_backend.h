#ifndef SDL_BACKEND_H_
#define SDL_BACKEND_H_

#include "error/error.h"

/**
 * Initiate the SDL "backend".
 *
 * \param[in] p_engine Double pointer to SDL_Window to be initiated.
 *
 * \return True if successful, else false.
 */
error_t sdl_backend_init(struct SDL_Window** pp_window, int width, int height);

/**
 * \brief Destroy SDL backend.
 *
 * \param[in] p_void_window Pointer to SDL window to be destroyed.
 */
void sdl_backend_destroy(void* p_void_window);

#endif // SDL_BACKEND_H_
