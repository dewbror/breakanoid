#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "error/vulkan_error.h"
#include "logger.h"

#include "util/deletion_stack.h"

#include "vulkan/vulkan_types.h"
#include "vulkan/vulkan_cmd.h"

/**
 * Struct for deleting a command pool.
 */
typedef struct cmd_pool_del_s {
    VkDevice device;
    VkCommandPool cmd_pool;
} cmd_pool_del_t;

/**
 * Deinit the command pools.
 */
void vulkan_cmd_pool_deinit(void* p_void_cmd_pool_del_struct);

error_t vulkan_cmd_frame_init(deletion_stack_t* p_dstack, VkDevice device, const queue_family_data_t* p_queues,
    frame_data_t* p_frames)
{
    if(device == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: device is NULL", __func__);

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

        if(vkAllocateCommandBuffers(device, &render_cmd_alloc_info, &p_frames[i].cmd) != VK_SUCCESS) {
            return error_init(ERR_SRC_VULKAN, VULKAN_ERR_CMD_BUF, "Failed to create frame command buffer");
        }
    }

    // CLEANUP
    for(int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        cmd_pool_del_t* p_cmd_pool = (cmd_pool_del_t*)malloc(sizeof(cmd_pool_del_t));
        p_cmd_pool->device = device;
        p_cmd_pool->cmd_pool = p_frames[i].cmd_pool;

        error_t err = deletion_stack_push(p_dstack, p_cmd_pool, vulkan_cmd_pool_deinit);
        if(err.code != 0) {
            vulkan_cmd_pool_deinit(p_cmd_pool);
            return err;
        }
    }

    LOG_DEBUG("%s: Successful", __func__);

    return SUCCESS;
}

error_t vulkan_cmd_imm_init(deletion_stack_t* p_dstack, VkDevice device, const queue_family_data_t* p_queues, VkCommandPool* p_imm_cmd_pool,
    VkCommandBuffer* p_imm_cmd_buff)
{
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

    // CLEANUP
    cmd_pool_del_t* p_cmd_pool = (cmd_pool_del_t*)malloc(sizeof(cmd_pool_del_t));
    p_cmd_pool->device = device;
    p_cmd_pool->cmd_pool = *p_imm_cmd_pool;

    error_t err = deletion_stack_push(p_dstack, p_cmd_pool, vulkan_cmd_pool_deinit);
    if(err.code != 0) {
        vulkan_cmd_pool_deinit(p_cmd_pool);
        return err;
    }

    LOG_DEBUG("%s: Successful", __func__);

    return SUCCESS;
}

void vulkan_cmd_pool_deinit(void* p_void_cmd_del)
{
    LOG_DEBUG("Callback: %s", __func__);

    if(p_void_cmd_del == NULL) {
        LOG_ERROR("%s: p_void_cmd_del_struct is NULL", __func__);
        return;
    }

    // Cast pointer
    cmd_pool_del_t* p_cmd_pool_del = (cmd_pool_del_t*)p_void_cmd_del;

    // NULL check struct fields

    vkDestroyCommandPool(p_cmd_pool_del->device, p_cmd_pool_del->cmd_pool, VK_NULL_HANDLE);

    free(p_cmd_pool_del);
    p_cmd_pool_del = NULL;
    p_void_cmd_del = NULL;
}

VkCommandBufferSubmitInfo vulkan_cmd_get_buffer_submit_info(VkCommandBuffer cmd)
{
    VkCommandBufferSubmitInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info.commandBuffer = cmd;
    info.deviceMask = 0;

    return info;
}

VkSubmitInfo2 vulkan_cmd_get_submit_info2(const VkCommandBufferSubmitInfo* cmd_buffer_submit_info,
    const VkSemaphoreSubmitInfo* signal_semaphore_submit_info, const VkSemaphoreSubmitInfo* wait_semaphore_sbmit_info)
{
    // We will be using vkQueueSubmit2 for submitting our commands. This is part of syncronization-2 and is an updated
    // version of the older VkQueueSubmit from vulkan 1.0. The function call requires a VkSubmitInfo2 which contains the
    // information on the semaphores used as part of the submit, and we can give it a Fence so that we can check for
    // that submit to be finished executing. VkSubmitInfo2 requires VkSemaphoreSubmitInfo for each of the semaphores it
    // uses, and a VkCommandBufferSubmitInfo for the command buffers that will be enqueued as part of the submit. Lets
    // check the vkinit functions for those.

    VkSubmitInfo2 info = {0};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    info.waitSemaphoreInfoCount = wait_semaphore_sbmit_info == VK_NULL_HANDLE ? 0 : 1;
    info.pWaitSemaphoreInfos = wait_semaphore_sbmit_info;
    info.signalSemaphoreInfoCount = signal_semaphore_submit_info == VK_NULL_HANDLE ? 0 : 1;
    info.pSignalSemaphoreInfos = signal_semaphore_submit_info;
    info.commandBufferInfoCount = 1;
    info.pCommandBufferInfos = cmd_buffer_submit_info;

    return info;
}
