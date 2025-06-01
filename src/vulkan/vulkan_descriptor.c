#include <stdbool.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "error/vulkan_error.h"
#include "logger.h"
#include "util/deletion_stack.h"
#include "vulkan/vulkan_types.h"
#include "vulkan/vulkan_descriptor.h"

/**
 * Structure for deletion of VkDescriptorSetLayout and VkDescriptorPool.
 */
typedef struct desc_del_s {
    VkDevice device;
    VkDescriptorSetLayout desc_layout;
    VkDescriptorPool pool;
} desc_del_t;

static bool pool_init(VkDevice device, uint32_t max_sets, pool_size_ratio_t* p_pool_ratios, size_t pool_ratios_count,
    VkDescriptorPool* p_pool);

static void vulkan_descriptor_deinit(void* p_void_vulkan_desc_del);

error_t vulkan_descriptor_init(deletion_stack_t* p_dstack, VkDevice device, allocated_image_t* p_draw_image,
    descriptor_allocator_t* p_descriptor_allocator, VkDescriptorSet* p_draw_image_desc,
    VkDescriptorSetLayout* p_draw_image_desc_layout)
{
    pool_size_ratio_t sizes = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1};
    if(!pool_init(device, 10, &sizes, 1, &p_descriptor_allocator->pool))
        return error_init(ERR_SRC_CORE, ERR_TEMP, "Failed to init pool");

    uint32_t bindings_count = 1;
    VkDescriptorSetLayoutBinding p_bindings[1];

    VkDescriptorSetLayoutBinding new_bind = {0};
    new_bind.binding = 0;
    new_bind.descriptorCount = 1;
    new_bind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

    p_bindings[0] = new_bind;

    for(uint32_t i = 0; i < bindings_count; ++i) {
        p_bindings[i].stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layout_info = {0};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.pBindings = p_bindings;
    layout_info.bindingCount = bindings_count;
    layout_info.flags = 0;

    if(vkCreateDescriptorSetLayout(device, &layout_info, VK_NULL_HANDLE, p_draw_image_desc_layout) != VK_SUCCESS) {
        return error_init(ERR_SRC_VULKAN, VULKAN_ERR_CREATE_DESCRIPTOR_SET_LAYOUT,
            "Failed to create descriptor set layout");
    }

    VkDescriptorSetAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = p_descriptor_allocator->pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = p_draw_image_desc_layout;

    if(vkAllocateDescriptorSets(device, &alloc_info, p_draw_image_desc) != VK_SUCCESS) {
        return error_init(ERR_SRC_VULKAN, VULKAN_ERR_ALLOCATE_DESCRIPTOR_SETS, "Failed to allocate descriptor set");
    }

    VkDescriptorImageInfo img_info = {0};
    img_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    img_info.imageView = p_draw_image->image_view;

    VkWriteDescriptorSet draw_image_write = {0};
    draw_image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    draw_image_write.dstBinding = 0;
    draw_image_write.dstSet = *p_draw_image_desc;
    draw_image_write.descriptorCount = 1;
    draw_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    draw_image_write.pImageInfo = &img_info;

    vkUpdateDescriptorSets(device, 1, &draw_image_write, 0, VK_NULL_HANDLE);

    // CLEANUP
    desc_del_t* p_desc_del = (desc_del_t*)malloc(sizeof(desc_del_t));
    p_desc_del->device = device;
    p_desc_del->pool = p_descriptor_allocator->pool;
    p_desc_del->desc_layout = *p_draw_image_desc_layout;

    error_t err = deletion_stack_push(p_dstack, p_desc_del, vulkan_descriptor_deinit);
    if(err.code != 0) {
        vulkan_descriptor_deinit(p_desc_del);
        return err;
    }

    return SUCCESS;
}

static void vulkan_descriptor_deinit(void* p_void_desc_del)
{
    LOG_DEBUG("Callback: %s", __func__);

    // Cast pointer
    desc_del_t* p_desc_del = (desc_del_t*)p_void_desc_del;

    vkDestroyDescriptorPool(p_desc_del->device, p_desc_del->pool, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(p_desc_del->device, p_desc_del->desc_layout, VK_NULL_HANDLE);

    free(p_desc_del);
    p_desc_del = NULL;
    p_void_desc_del = NULL;
}

static bool pool_init(VkDevice device, uint32_t max_sets, pool_size_ratio_t* p_pool_ratios,
    const size_t pool_ratios_count, VkDescriptorPool* p_pool)
{
    LOG_DEBUG("%s", __func__);

    VkDescriptorPoolSize* p_pool_sizes = (VkDescriptorPoolSize*)malloc(
        pool_ratios_count * sizeof(VkDescriptorPoolSize));
    if(p_pool_sizes == NULL) {
        LOG_ERROR("%s: Failed to allocate memory of size %lu", __func__,
            pool_ratios_count * sizeof(VkDescriptorPoolSize));
        return false;
    }

    for(size_t i = 0; i < pool_ratios_count; ++i) {
        p_pool_sizes[i].type = p_pool_ratios->type;
        p_pool_sizes[i].descriptorCount = (uint32_t)p_pool_ratios->ratio * max_sets;
    }

    VkDescriptorPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = max_sets;
    pool_info.poolSizeCount = (uint32_t)pool_ratios_count;
    pool_info.pPoolSizes = p_pool_sizes;

    if(vkCreateDescriptorPool(device, &pool_info, VK_NULL_HANDLE, p_pool) != VK_SUCCESS) {
        LOG_ERROR("%s: Failed to create descriptor pool", __func__);
        return false;
    }

    free(p_pool_sizes);
    p_pool_sizes = NULL;

    LOG_DEBUG("descriptor pools created");

    return true;
}
