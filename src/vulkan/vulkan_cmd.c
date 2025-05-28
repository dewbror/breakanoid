#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "error/vulkan_error.h"
#include "logger.h"
#include "vulkan/vulkan_types.h"
#include "vulkan/vulkan_cmd.h"

error_t vulkan_cmd_frame_init(VkDevice device, queue_family_data_t* p_queues, frame_data_t* p_frames) {
    if(device == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: p_engine is NULL", __func__);

    if(p_queues == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: p_queues is NULL", __func__);

    // Create a command pool for commands submitted to the graphics queue
    // We also want the pool to allow for resetting of individual command buffers

    VkCommandPoolCreateInfo cmd_pool_info = {0};

    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_info.queueFamilyIndex = p_queues->graphics_index;

    // Allocate the default command buffer that we will use for rendering
    VkCommandBufferAllocateInfo render_cmd_alloc_info = {0};

    render_cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    render_cmd_alloc_info.commandBufferCount = 1;
    render_cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    for(int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        if(vkCreateCommandPool(device, &cmd_pool_info, VK_NULL_HANDLE, &p_frames[i].cmd_pool) != VK_SUCCESS) {
            return error_init(ERR_SRC_VULKAN, VULKAN_ERR_CMD_POOL, "Failed to create frame command pool");
        }

        render_cmd_alloc_info.commandPool = p_frames[i].cmd_pool;

        if(vkAllocateCommandBuffers(device, &render_cmd_alloc_info, &p_frames[i].main_cmd_buffer) != VK_SUCCESS) {
            return error_init(ERR_SRC_VULKAN, VULKAN_ERR_CMD_BUF, "Failed to create frame command buffer");
        }
    }

    LOG_INFO("Frame command structures created");

    return SUCCESS;
}

error_t vulkan_cmd_imm_init(
    VkDevice device, const queue_family_data_t* p_queues, VkCommandPool* p_imm_cmd_pool, VkCommandBuffer* p_imm_cmd_buff
) {
    if(device == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: device is NULL", __func__);

    if(p_queues == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: p_queues is NULL", __func__);

    // Create immediate submit command pool and command buffer
    VkCommandPoolCreateInfo cmd_pool_info = {0};

    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_info.queueFamilyIndex = p_queues->graphics_index;

    if(vkCreateCommandPool(device, &cmd_pool_info, VK_NULL_HANDLE, p_imm_cmd_pool) != VK_SUCCESS)
        return error_init(ERR_SRC_VULKAN, VULKAN_ERR_CMD_POOL, "Failed to create immediate command pool");

    VkCommandBufferAllocateInfo imm_cmd_alloc_info = {0};

    imm_cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    imm_cmd_alloc_info.commandBufferCount = 1;
    imm_cmd_alloc_info.commandPool = *p_imm_cmd_pool;
    imm_cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    if(vkAllocateCommandBuffers(device, &imm_cmd_alloc_info, p_imm_cmd_buff) != VK_SUCCESS)
        return error_init(ERR_SRC_VULKAN, VULKAN_ERR_CMD_BUF, "Failed to create frame command buffer");

    LOG_INFO("Immidiate command structures created");

    return SUCCESS;
}

void vulkan_cmd_pool_destroy(void* p_void_cmd_del_struct) {
    LOG_DEBUG("Callback: vulkan_cmd_pool_destroy");

    if(p_void_cmd_del_struct == NULL) {
        LOG_ERROR("%s: p_void_cmd_del_struct is NULL", __func__);
        return;
    }

    // Cast pointer
    cmd_pool_del_struct_t* p_cmd_pool_del_struct = (cmd_pool_del_struct_t*)p_void_cmd_del_struct;

    // NULL check struct fields

    vkDestroyCommandPool(p_cmd_pool_del_struct->device, p_cmd_pool_del_struct->cmd_pool, VK_NULL_HANDLE);

    free(p_cmd_pool_del_struct);
    p_cmd_pool_del_struct = NULL;
    p_void_cmd_del_struct = NULL;
}
