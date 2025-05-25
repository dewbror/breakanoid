#ifndef VULKAN_SWAPCHAIN_H_
#define VULKAN_SWAPCHAIN_H_

#include <stdbool.h>
#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_core.h>

#include "vulkan/vulkan_types.h"

/**
 * Struct used for deleting a debug messenger.
 */
typedef struct vulkan_swapchain_del_struct_s {
    VkDevice device;
    vulkan_swapchain_t vulkan_swapchain;
} vulkan_swapchain_del_struct_t;

bool vulkan_swapchain_init(
    VkSurfaceKHR surface, SDL_Window* p_window, VkPhysicalDevice physical_device, VkDevice device,
    vulkan_swapchain_t* p_vulkan_swapchain);

void vulkan_swapchain_destroy(void* p_void_vulkan_swapchain_del_struct);

#endif // VULKAN_SWAPCHAIN_H_
