#ifndef VULKAN_PIPELINE_H_
#define VULKAN_PIPELINE_H_

#include <vulkan/vulkan_core.h>

#include "error/error.h"

typedef struct vulkan_pipeline_del_s {
    VkDevice device;
    VkPipelineLayout layout;
    VkPipeline pipeline;
} vulkan_pipeline_del_t;

error_t vulkan_pipeline_init(VkDevice device, VkPhysicalDevice physical_device, VkExtent2D window_extent,
    VkDescriptorSetLayout* p_draw_image_desc_layout, VkPipelineLayout* p_gradient_pipeline_layout,
    VkPipeline* p_gradient_pipeline);

void vulkan_pipeline_destroy(void* p_void_vulkan_pipeline_del);

#endif // VULKAN_PIPELINE_H_
