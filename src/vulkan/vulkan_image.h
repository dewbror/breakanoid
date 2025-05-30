#ifndef VULKAN_IMAGE_H_
#define VULKAN_IMAGE_H_

#include <stdbool.h>

#include "error/error.h"

#include "vulkan/vulkan_types.h"

typedef struct allocated_image_del_struct_s {
    VkDevice device;
    allocated_image_t allocated_image;
} allocated_image_del_strut_t;

/**
 * \brief Create vulkan image.
 */
error_t vulkan_image_create(VkDevice device, VkPhysicalDevice physical_device, uint32_t width, uint32_t height,
    allocated_image_t* p_allocated_image);

/**
 * Destroy vulkan image.
 */
void vulkan_image_destroy(void* p_void_allocated_image_del_struct);

void vulkan_image_transition(VkCommandBuffer cmd, VkImage img, VkImageLayout old_layout, VkImageLayout new_layout);

void vulkan_image_copy_image_to_image(VkCommandBuffer cmd, VkImage src_img, VkImage dst_img, VkExtent2D src_ext,
    VkExtent2D dst_ext);

#endif // VULKAN_IMAGE_H_
