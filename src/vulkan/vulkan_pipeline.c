#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "error/vulkan_error.h"

#include "logger.h"
#include "vulkan/vulkan_pipeline.h"

static error_t background_pipeline_init(VkDevice device, VkPhysicalDevice physical_device, VkExtent2D window_extent,
    VkDescriptorSetLayout* p_draw_image_desc_layout, VkPipelineLayout* p_gradient_pipeline_layout,
    VkPipeline* p_gradient_pipeline);

error_t vulkan_pipeline_init(VkDevice device, VkPhysicalDevice physical_device, VkExtent2D window_extent,
    VkDescriptorSetLayout* p_draw_image_desc_layout, VkPipelineLayout* p_gradient_pipeline_layout,
    VkPipeline* p_gradient_pipeline) {
    error_t err = background_pipeline_init(device, physical_device, window_extent, p_draw_image_desc_layout,
        p_gradient_pipeline_layout, p_gradient_pipeline);
    if(err.code != 0)
        return err;

    LOG_INFO("Vulkan pipeline initiated");

    return SUCCESS;
}

static error_t background_pipeline_init(VkDevice device, VkPhysicalDevice physical_device, VkExtent2D window_extent,
    VkDescriptorSetLayout* p_draw_image_desc_layout, VkPipelineLayout* p_gradient_pipeline_layout,
    VkPipeline* p_gradient_pipeline) {
    VkPhysicalDeviceProperties properties = {0};
    vkGetPhysicalDeviceProperties(physical_device, &properties);

    double group_count_x = window_extent.width / 32.0;
    double group_count_y = window_extent.height / 33.0;

    LOG_DEBUG("group_count_x: %g", group_count_x);
    LOG_DEBUG("group_count_y: %g", group_count_y);

    VkPipelineLayoutCreateInfo comp_layout_info = {0};
    comp_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    comp_layout_info.pSetLayouts = p_draw_image_desc_layout;
    comp_layout_info.setLayoutCount = 1;

    if(vkCreatePipelineLayout(device, &comp_layout_info, VK_NULL_HANDLE, p_gradient_pipeline_layout) != VK_SUCCESS)
        return error_init(ERR_SRC_VULKAN, VULKAN_ERR_CREATE_PIPELINE_LAYOUT, "Failed to create Vulkan pipeline layout");

    LOG_INFO("Background pipeline layout created");

    LOG_DEBUG("Creating compute shader module");
    LOG_DEBUG("Opening shader file");

    FILE* shader_file = fopen("../src/shaders/comp.spv", "rb");
    if(shader_file == NULL)
        return error_init(ERR_SRC_CORE, ERR_FOPEN, "Failed to open file: ../src/shaders/comp.spv: %s", strerror(errno));

    int ret = fseek(shader_file, 0, SEEK_END);
    if(ret != 0)
        return error_init(ERR_SRC_CORE, ERR_FSEEK, "fseek failed");

    long size = ftell(shader_file);
    if(size < 0)
        return error_init(ERR_SRC_CORE, ERR_FTELL, "ftell failed");
    size_t code_size = (size_t)size;

    ret = fseek(shader_file, 0, SEEK_SET);
    if(ret != 0)
        return error_init(ERR_SRC_CORE, ERR_FSEEK, "fseek failed");

    LOG_DEBUG("File size: %ld", size);

    // If file size is not a multiple of sizeof(uint32_t) = 4
    if((size_t)size % sizeof(uint32_t) != 0)
        return error_init(ERR_SRC_CORE, ERR_TEMP, "temp");

    uint32_t* p_buf = (uint32_t*)malloc(code_size);
    if(p_buf == NULL)
        return error_init(
            ERR_SRC_CORE, ERR_MALLOC, "Failed to allocate memory of size %lu", (size_t)size * sizeof(char));

    size_t ret_LU = fread(p_buf, 1, code_size, shader_file);
    if(ret_LU < code_size)
        return error_init(ERR_SRC_CORE, ERR_FREAD, "fread failed");

    ret = fclose(shader_file);
    if(ret != 0)
        return error_init(ERR_SRC_CORE, ERR_FCLOSE, "fclose failed");

    LOG_DEBUG("Shader file successfully read");

    VkShaderModuleCreateInfo shader_info = {0};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = code_size;
    shader_info.pCode = p_buf;

    VkShaderModule comp_draw_shader = NULL;
    if(vkCreateShaderModule(device, &shader_info, VK_NULL_HANDLE, &comp_draw_shader) != VK_SUCCESS)
        return error_init(ERR_SRC_VULKAN, VULKAN_ERR_CREATE_SHADER_MODULE, "Failed to create shader module");

    free(p_buf);
    p_buf = NULL;

    LOG_DEBUG("Shader module created");

    VkSpecializationMapEntry entries[3];
    entries[0].constantID = 0; // matches local_size_x_id in GLSL
    entries[0].offset = 0;
    entries[0].size = sizeof(uint32_t);

    entries[1].constantID = 1; // matches local_size_x_id in GLSL
    entries[1].offset = sizeof(uint32_t);
    entries[1].size = sizeof(uint32_t);

    entries[2].constantID = 2; // matches local_size_x_id in GLSL
    entries[2].offset = 2 * sizeof(uint32_t);
    entries[2].size = sizeof(uint32_t);

    uint32_t spec_data[3] = {32, 32, 1}; // 32x32x1 working group size

    VkSpecializationInfo spec_info = {0};
    spec_info.mapEntryCount = 3;
    spec_info.pMapEntries = entries;
    spec_info.dataSize = sizeof(spec_data);
    spec_info.pData = spec_data;

    VkPipelineShaderStageCreateInfo stage_info = {0};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_info.module = comp_draw_shader;
    stage_info.pName = "main"; // Shader entry point?
    stage_info.pSpecializationInfo = &spec_info;

    VkComputePipelineCreateInfo comp_pipeline_info = {0};
    comp_pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    comp_pipeline_info.layout = *p_gradient_pipeline_layout;
    comp_pipeline_info.stage = stage_info;

    if(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &comp_pipeline_info, VK_NULL_HANDLE, p_gradient_pipeline) !=
        VK_SUCCESS)
        return error_init(ERR_SRC_VULKAN, VULKAN_ERR_CREATE_COMPUTE_PIPELINES, "Failed to create compute pipelines");

    LOG_DEBUG("Compute pipeline created");

    // Safe to destroy after pipline has been created
    vkDestroyShaderModule(device, comp_draw_shader, VK_NULL_HANDLE);

    LOG_INFO("Vulkan background pipeline initiated");

    return SUCCESS;
}

void vulkan_pipeline_destroy(void* p_void_vulkan_pipeline_del) {
    LOG_DEBUG("Callback: %s", __func__);

    // Cast pointer
    vulkan_pipeline_del_t* p_vulkan_pipeline_del = (vulkan_pipeline_del_t*)p_void_vulkan_pipeline_del;

    vkDestroyPipelineLayout(p_vulkan_pipeline_del->device, p_vulkan_pipeline_del->layout, VK_NULL_HANDLE);
    vkDestroyPipeline(p_vulkan_pipeline_del->device, p_vulkan_pipeline_del->pipeline, VK_NULL_HANDLE);

    free(p_vulkan_pipeline_del);
    p_vulkan_pipeline_del = NULL;
    p_void_vulkan_pipeline_del = NULL;
}
