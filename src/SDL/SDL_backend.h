#ifndef SDL_BACKEND_H_
#define SDL_BACKEND_H_
#pragma once

/**
 * SDL_backend.h
 */

#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"
#include "vulkan/vulkan_engine.h"

bool init_SDL_backend(vulkan_engine *p_engine);
#endif // SDL_BACKEND_H_
