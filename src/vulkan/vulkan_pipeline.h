#ifndef VULKAN_PIPELINE_H_
#define VULKAN_PIPELINE_H_

#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "util/deletion_stack.h"

error_t vulkan_pipeline_init(deletion_stack_t* p_dstack, VkDevice device, VkPhysicalDevice physical_device, VkExtent2D window_extent,
    VkDescriptorSetLayout* p_draw_image_desc_layout, VkPipelineLayout* p_gradient_pipeline_layout,
    VkPipeline* p_gradient_pipeline);

#endif // VULKAN_PIPELINE_H_
