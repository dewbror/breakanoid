#ifndef VULKAN_SWAPCHAIN_H_
#define VULKAN_SWAPCHAIN_H_

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_core.h>

#include "error/error.h"

#include "vulkan/vulkan_types.h"

/**
 * Struct used for deleting a debug messenger.
 */
typedef struct vulkan_swapchain_del_struct_s {
    VkDevice device;
    vulkan_swapchain_t vulkan_swapchain;
} vulkan_swapchain_del_struct_t;

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
error_t vulkan_swapchain_init(
    VkDevice device, VkPhysicalDevice physical_device, VkSurfaceKHR surface, SDL_Window* p_window,
    vulkan_swapchain_t* p_vulkan_swapchain
);

/**
 * \brief Destroy vulkan swapchain.
 *
 * \param[in] p_void_vulkan_swapchain_del_struct Pointer to the vulkan_swapchain_del_struct_t containing the
 * swapchain to be deleted.
 */
void vulkan_swapchain_destroy(void* p_void_vulkan_swapchain_del_struct);

#endif // VULKAN_SWAPCHAIN_H_
