#ifndef VULKAN_TYPES_H_
#define VULKAN_TYPES_H_

#include <vulkan/vulkan_core.h>

#define FRAMES_IN_FLIGHT 2

/**
 * Struct for holding the queue family information.
 */
typedef struct queue_family_data_s {
    // Vulkan opaque pointers 8/4 bytes
    VkQueue graphics;
    VkQueue present;

    // uint32_t 4 bytes
    uint32_t graphics_index;
    uint32_t present_index;
} queue_family_data_t;

/**
 * Struct used for finding the swapchain support details.
 */
typedef struct swapchain_support_details_s {
    // Vulkan opaque pointers 8/4 bytes
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    VkPresentModeKHR* present_modes;

    // unsigned long >= 4 bytes
    size_t formats_count;
    size_t present_modes_count;
} swapchain_support_details_t;

/**
 * A struct containing everything relevant for the swapchain.
 */
typedef struct vulkan_swapchain_s {
    // Vulkan opaque pointers 8/4 bytes
    VkSwapchainKHR swapchain;
    VkImage* p_images;
    VkImageView* p_image_views;
    VkFormat format;
    VkExtent2D extent;

    // uint32_t 4 bytes
    uint32_t images_count;
} vulkan_swapchain_t;

/**
 * Struct containing all data relevant for an image allocated on the physical device
 */
typedef struct allocated_image_s {
    VkImage image;
    VkImageView image_view;
    // VmaAllocation alloc;
    VkDeviceMemory mem;
    VkExtent3D extent;
    VkFormat format;
} allocated_image_t;

/**
 * A struct for holden per frame data and vulkan handles
 */
typedef struct frame_data_s {
    VkCommandPool cmd_pool;
    VkCommandBuffer main_cmd_buffer;
    VkSemaphore swapchain_semaphore;
    VkSemaphore render_semaphore;
    VkFence render_fence;
    struct deletion_queue_s* p_delq;
} frame_data_t;

typedef struct pool_size_ratio_s {
    VkDescriptorType type;
    float ratio;
} pool_size_ratio_t;

typedef struct descriptor_allocator_s {
    pool_size_ratio_t pool_size_ratio;
    VkDescriptorPool pool;
} descriptor_allocator_t;

#endif // VULKAN_TYPES_H_
