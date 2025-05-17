/*
  SDL_backend.h
*/

#ifndef SDL_BACKEND_H_
#define SDL_BACKEND_H_
#pragma once

#include <stdbool.h>

/**
 * Initiate the SDL "backend".
 *
 * \param[in] p_engine Pointer to the vulkan engine.
 *
 * \return True if successful, false if failed.
 */
bool init_SDL_backend(struct vulkan_engine_s* p_engine);
#endif // SDL_BACKEND_H_
