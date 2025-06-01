#ifndef VULKAN_CONTEXT_H_
#define VULKAN_CONTEXT_H_

#include <stdbool.h>

#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "vulkan/vulkan_types.h"

/**
 * A struct containing all the necessary vulkan fields.
 */
typedef struct vulkan_context_s {
    struct deletion_stack_s* p_dstack; // Make embedded struct?
    struct SDL_Window* p_window;
    VkExtent2D window_extent;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_msg;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;
    queue_family_data_t queues;
    VkSampleCountFlagBits msaa_samples;
    vulkan_swapchain_t vulkan_swapchain;
    allocated_image_t draw_image;
    VkExtent2D draw_extent;
    long frame_count;
    frame_data_t p_frames[FRAMES_IN_FLIGHT];
    VkCommandPool imm_cmd_pool;
    VkCommandBuffer imm_cmd_buffer;
    VkFence imm_fence;
    descriptor_allocator_t desc_alloc;
    VkDescriptorSet draw_img_desc;
    VkDescriptorSetLayout draw_img_desc_layout;
    VkPipeline gradient_pipline;
    VkPipelineLayout gradient_pipline_layout;
} vulkan_context_t;

/**
 * Initiate the vulkan context.
 *
 * \param[in] p_vkctx Pointer to the vulkan_context to be initiated. Must be deleted using vulkan_deinit before
 * exiting game.
 *
 * \return True if successful, false if failed.
 */
error_t vulkan_init(vulkan_context_t* p_vkctx);

/**
 * Used to delete a vulkan_engine. All deletion/destruction of objects is currently handled by the deletion queue. All
 * this function does currently is flush the deletion queue.
 *
 * \param[in] p_vkctx Pointer to the vulkan_context.
 *
 * \return True if successful, false otherwise
 */
error_t vulkan_deinit(vulkan_context_t* p_vkctx);

void vulkan_render_and_present_frame(vulkan_context_t* p_vkctx);

#endif // VULKAN_CONTEXT_H_
