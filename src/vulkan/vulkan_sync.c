#include <stdbool.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "error/vulkan_error.h"

#include "logger.h"
#include "vulkan/vulkan_types.h"
#include "vulkan/vulkan_sync.h"

error_t vulkan_sync_frame_init(VkDevice device, frame_data_t* p_frames)
{
    if(device == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: device is NULL", __func__);

    if(p_frames == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: p_frames is NULL", __func__);

    // Create sync structures

    // One fence to control when the GPU has finished rendering the frame and 2 semaphores to sync rendering with the
    // swapchain. We want the fence to start signaled so we can wait on it the first time.

    VkFenceCreateInfo fence_info = {0};

    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo sem_info = {0};

    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem_info.flags = 0;

    for(int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        if(vkCreateFence(device, &fence_info, VK_NULL_HANDLE, &p_frames[i].render_fence) != VK_SUCCESS)
            return error_init(ERR_SRC_VULKAN, VULKAN_ERR_FENCE, "Failed to create render fence");

        if(vkCreateSemaphore(device, &sem_info, VK_NULL_HANDLE, &p_frames[i].render_semaphore) != VK_SUCCESS)
            return error_init(ERR_SRC_VULKAN, VULKAN_ERR_SEMAPHORE, "Failed to create render semaphore");

        if(vkCreateSemaphore(device, &sem_info, VK_NULL_HANDLE, &p_frames[i].swapchain_semaphore) != VK_SUCCESS)
            return error_init(ERR_SRC_VULKAN, VULKAN_ERR_SEMAPHORE, "Failed to swapchain render semaphore");
    }

    LOG_INFO("Frame sync structures created");

    return SUCCESS;
}

void vulkan_sync_frame_destroy(void* p_void_vulkan_sync_frame_del_struct)
{
    LOG_DEBUG("Callback: %s", __func__);

    if(p_void_vulkan_sync_frame_del_struct == NULL) {
        LOG_ERROR("%s: p_void_vulkan_sync_frame_del_struct is NULL", __func__);
        return;
    }

    // Cast pointer
    vulkan_sync_frame_del_struct_t* p_frame = (vulkan_sync_frame_del_struct_t*)p_void_vulkan_sync_frame_del_struct;

    // NULL check struct fields

    for(int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        LOG_DEBUG("    Destroying frame sync structs, index: %d", i);

        vkDestroyFence(p_frame->device, p_frame->p_frames[i].render_fence, VK_NULL_HANDLE);
        vkDestroySemaphore(p_frame->device, p_frame->p_frames[i].render_semaphore, VK_NULL_HANDLE);
        vkDestroySemaphore(p_frame->device, p_frame->p_frames[i].swapchain_semaphore, VK_NULL_HANDLE);
    }

    free(p_frame);
    p_frame = NULL;
    p_void_vulkan_sync_frame_del_struct = NULL;
}

error_t vulkan_sync_imm_init(VkDevice device, VkFence* p_imm_fence)
{
    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if(vkCreateFence(device, &fence_info, VK_NULL_HANDLE, p_imm_fence) != VK_SUCCESS)
        return error_init(ERR_SRC_VULKAN, VULKAN_ERR_FENCE, "Failed to create immediate fence");

    LOG_INFO("Immediate sync structures created");

    return SUCCESS;
}

void vulkan_sync_semaphore_destroy(void* p_void_semaphore_del_struct)
{
    LOG_DEBUG("Callback: %s", __func__);

    if(p_void_semaphore_del_struct == NULL) {
        LOG_ERROR("%s: p_void_vulkan_sync_frame_del_struct is NULL", __func__);
    }

    // Cast pointer
    semaphore_del_struct_t* p_semaphore_del_struct = (semaphore_del_struct_t*)p_void_semaphore_del_struct;

    // NULL check struct fields

    vkDestroySemaphore(p_semaphore_del_struct->device, p_semaphore_del_struct->sem, VK_NULL_HANDLE);

    free(p_semaphore_del_struct);
    p_semaphore_del_struct = NULL;
    p_void_semaphore_del_struct = NULL;
}

void vulkan_sync_fence_destroy(void* p_void_fence_del_struct)
{
    LOG_DEBUG("Callback: %s", __func__);

    if(p_void_fence_del_struct == NULL) {
        LOG_ERROR("%s: p_void_vulkan_sync_frame_del_struct is NULL", __func__);
    }
    // Cast pointer
    fence_del_struct_t* p_fence_del_struct = (fence_del_struct_t*)p_void_fence_del_struct;

    // NULL check struct fields?

    vkDestroyFence(p_fence_del_struct->device, p_fence_del_struct->fence, VK_NULL_HANDLE);

    free(p_fence_del_struct);
    p_fence_del_struct = NULL;
    p_void_fence_del_struct = NULL;
}

VkSemaphoreSubmitInfo vulkan_sync_get_sem_submit_info(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore)
{
    VkSemaphoreSubmitInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    info.semaphore = semaphore;
    info.stageMask = stage_mask;
    info.deviceIndex = 0;
    info.value = 1;

    return info;
}
