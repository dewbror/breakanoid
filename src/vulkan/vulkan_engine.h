#ifndef VULKAN_ENGINE_H_
#define VULKAN_ENGINE_H_

#include <stdbool.h>
#include <vulkan/vulkan_core.h>

#include "vulkan/vulkan_types.h"

#define FRAMES_IN_FLIGHT 2

typedef struct allocated_image_s {
    VkImage image;
    VkImageView image_view;
    // VmaAllocation alloc;
    VkDeviceMemory mem;
    VkExtent3D extent;
    VkFormat format;
} allocated_image_t;

typedef struct frame_data_s {
    VkCommandPool cmd_pool;
    VkCommandBuffer main_cmd_buffer;
    VkSemaphore swapchain_semaphore;
    VkSemaphore render_semaphore;
    VkFence render_fence;
    struct deletion_queue_s* p_delq;
} frame_data_t;

/**
 * A struct containing all the necessary vulkan fields.
 */
typedef struct vulkan_engine_s {
    struct deletion_stack_s* p_main_del_stack;
    struct SDL_Window* p_SDL_window;
    VkExtent2D window_extent;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_msg;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;
    queue_family_data_t queues;
    VkSampleCountFlagBits msaa_samples;
    struct vulkan_swapchain_s vulkan_swapchain;
    allocated_image_t draw_image;
    long frame_count;
    frame_data_t frames[FRAMES_IN_FLIGHT];
    VkCommandPool imm_cmd_pool;
    VkCommandBuffer imm_cmd_buffer;
    VkFence imm_fence;
} vulkan_engine_t;

/**
 * Initiate the vulkan engine.
 *
 * \param[in] p_engine Pointer to the vulkan_engine to be initiated. Must be deleted using vulkan_engine_destroy before
 * exiting game.
 *
 * \return True if successful, false if failed.
 */
bool vulkan_engine_init(vulkan_engine_t* p_engine);

/**
 * Used to delete a vulkan_engine. All deletion/destruction of objects is currently handled by the deletion queue. All
 * this function does currently is flush the deletion queue.
 *
 * \param[in] p_engine Pointer to the vulkan_engine.
 *
 * \return True if successful, false otherwise
 */
bool vulkan_engine_destroy(vulkan_engine_t* p_engine);
#endif // VULKAN_ENGINE_H_
