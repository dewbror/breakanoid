#ifndef VULKAN_SWAPCHAIN_H_
#define VULKAN_SWAPCHAIN_H_

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_core.h>

#include "error/error.h"

#include "util/deletion_stack.h"
#include "vulkan/vulkan_types.h"

/**
 * \brief Initaite swapchain.
 *
 * \param[in] device The Vulkan logical device.
 * \param[in] physical_device The physical device.
 * \param[in] surface The vulkan rendering surface.
 * \param[in] p_window Pointer to the SDL window.
 * \param[out] p_vulkan_swapchain Pointer to vulkan_swapchain_t containing the newly initiated swapchain and related
 * objects.
 * \return True if successful, else false.
 */
error_t vulkan_swapchain_init(deletion_stack_t* p_dstack, VkDevice device, VkPhysicalDevice physical_device,
    VkSurfaceKHR surface, SDL_Window* p_window, vulkan_swapchain_t* p_vulkan_swapchain);

#endif // VULKAN_SWAPCHAIN_H_
