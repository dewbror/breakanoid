#ifndef VULKAN_ENGINE_H_
#define VULKAN_ENGINE_H_

#include <stdbool.h>
#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "vulkan/vulkan_descriptor.h"
#include "vulkan/vulkan_types.h"

/**
 * A struct containing all the necessary vulkan fields.
 */
typedef struct vulkan_engine_s {
    struct deletion_stack_s* p_main_del_stack;
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
    frame_data_t p_frames[FRAMES_IN_FLIGHT]; // TODO: Rename to p_frames
    VkCommandPool imm_cmd_pool;
    VkCommandBuffer imm_cmd_buffer;
    VkFence imm_fence;
    descriptor_allocator_t desc_alloc;
    VkDescriptorSet draw_img_desc;
    VkDescriptorSetLayout draw_img_desc_layout;
    VkPipeline gradient_pipline;
    VkPipelineLayout gradient_pipline_layout;
} vulkan_engine_t;

/**
 * Initiate the vulkan engine.
 *
 * \param[in] p_engine Pointer to the vulkan_engine to be initiated. Must be deleted using vulkan_engine_destroy before
 * exiting game.
 *
 * \return True if successful, false if failed.
 */
error_t vulkan_engine_init(vulkan_engine_t* p_engine);

/**
 * Used to delete a vulkan_engine. All deletion/destruction of objects is currently handled by the deletion queue. All
 * this function does currently is flush the deletion queue.
 *
 * \param[in] p_engine Pointer to the vulkan_engine.
 *
 * \return True if successful, false otherwise
 */
error_t vulkan_engine_destroy(vulkan_engine_t* p_engine);

void vulkan_engine_render_and_present_frame(vulkan_engine_t* p_engine);

#endif // VULKAN_ENGINE_H_
