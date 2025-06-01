#ifndef VULKAN_DESCRIPTOR_H_
#define VULKAN_DESCRIPTOR_H_

#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "util/deletion_stack.h"
#include "vulkan/vulkan_types.h"

/**
 * Initiate
 */
error_t vulkan_descriptor_init(deletion_stack_t* p_dstack, VkDevice device, allocated_image_t* p_draw_image,
    descriptor_allocator_t* p_descriptor_allocator, VkDescriptorSet* p_draw_image_desc_set,
    VkDescriptorSetLayout* p_draw_image_desc_set_layout);


#endif // VULKAN_DESCRIPTOR_H_
