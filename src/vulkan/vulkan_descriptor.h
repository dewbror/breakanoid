#ifndef VULKAN_DESCRIPTOR_H_
#define VULKAN_DESCRIPTOR_H_

#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "vulkan/vulkan_types.h"

/**
 * Structure for deletion of VkDescriptorSetLayout and VkDescriptorPool.
 */
typedef struct vulkan_desc_del_s {
    VkDevice device;
    VkDescriptorSetLayout desc_layout;
    VkDescriptorPool pool;
} vulkan_desc_del_t;

/**
 * Initiate
 */
error_t vulkan_descriptor_init(VkDevice device, allocated_image_t* p_draw_image,
    descriptor_allocator_t* p_descriptor_allocator, VkDescriptorSet* p_draw_image_desc_set,
    VkDescriptorSetLayout* p_draw_image_desc_set_layout);

void vulkan_descriptor_destroy(void* p_void_vulkan_desc_del);

#endif // VULKAN_DESCRIPTOR_H_
