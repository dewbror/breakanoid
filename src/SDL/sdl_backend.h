#ifndef SDL_BACKEND_H_
#define SDL_BACKEND_H_

#include "error/error.h"
#include "util/deletion_stack.h"

/**
 * Initiate the SDL subsystem and create an SDL window.
 *
 * \param[in] p_del_stack Pointer to deletion stack.
 * \param[in] width The width of the SDL window.
 * \param[in] height The height of the SDL window.
 * \param[out] Double pointer to the SDL window that is created.
 *
 * \return True if successful, else false.
 */
error_t sdl_backend_init(struct deletion_stack_s* p_del_stack, int width, int height, struct SDL_Window** pp_window);

#endif // SDL_BACKEND_H_
