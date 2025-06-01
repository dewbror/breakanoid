#ifndef IMGUI_H_
#define IMGUI_H_

#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "util/deletion_stack.h"

error_t imgui_init(deletion_stack_t* p_dstack, VkInstance instance, VkPhysicalDevice physical_device, VkDevice device,
    struct SDL_Window* p_window, VkQueue graphics_queue, VkFormat* p_swapchain_format);

error_t imgui_draw(VkCommandBuffer cmd, VkImageView tgt_img_view, VkExtent2D swapchain_extent);

#endif // IMGUI_H_
