#include <stdbool.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "error/vulkan_error.h"
#include "logger.h"
#include "util/deletion_stack.h"
#include "vulkan/vulkan_types.h"
#include "vulkan/vulkan_sync.h"

typedef struct sync_frame_del_s {
    VkDevice device;
    frame_data_t* p_frames;
} sync_frame_del_t;

typedef struct semaphore_del_s {
    VkDevice device;
    VkSemaphore sem;
} semaphore_del_t;

typedef struct fence_del_s {
    VkDevice device;
    VkFence fence;
} fence_del_t;

static void vulkan_sync_frame_deinit(void* p_void_vulkan_sync_frame_del_struct);

static void vulkan_sync_semaphore_deinit(void* p_void_semaphore_del_struct);

static void vulkan_sync_fence_deinit(void* p_void_fence_del_struct);

error_t vulkan_sync_frame_init(deletion_stack_t* p_dstack, VkDevice device, frame_data_t* p_frames)
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

    // CLEANUP
    sync_frame_del_t* p_frame = malloc(sizeof(sync_frame_del_t));
    p_frame->device = device;
    p_frame->p_frames = p_frames;

    error_t err = deletion_stack_push(p_dstack, p_frame, vulkan_sync_frame_deinit);
    if(err.code != 0) {
        vulkan_sync_frame_deinit(p_frame);
        return err;
    }

    LOG_INFO("Frame sync structures initiated");

    return SUCCESS;
}

static void vulkan_sync_frame_deinit(void* p_void_sync_frame_del)
{
    LOG_DEBUG("Callback: %s", __func__);

    if(p_void_sync_frame_del == NULL) {
        LOG_ERROR("%s: p_void_vulkan_sync_frame_del_struct is NULL", __func__);
        return;
    }

    // Cast pointer
    sync_frame_del_t* p_frame_del = (sync_frame_del_t*)p_void_sync_frame_del;

    // NULL check struct fields

    for(int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        LOG_DEBUG("    Destroying frame sync structs, index: %d", i);

        vkDestroyFence(p_frame_del->device, p_frame_del->p_frames[i].render_fence, VK_NULL_HANDLE);
        vkDestroySemaphore(p_frame_del->device, p_frame_del->p_frames[i].render_semaphore, VK_NULL_HANDLE);
        vkDestroySemaphore(p_frame_del->device, p_frame_del->p_frames[i].swapchain_semaphore, VK_NULL_HANDLE);
    }

    free(p_frame_del);
    p_frame_del = NULL;
    p_void_sync_frame_del = NULL;
}

error_t vulkan_sync_imm_init(deletion_stack_t* p_dstack, VkDevice device, VkFence* p_imm_fence)
{
    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if(vkCreateFence(device, &fence_info, VK_NULL_HANDLE, p_imm_fence) != VK_SUCCESS)
        return error_init(ERR_SRC_VULKAN, VULKAN_ERR_FENCE, "Failed to create immediate fence");

    // CLEANUP
    fence_del_t* p_fence_del = (fence_del_t*)malloc(sizeof(fence_del_t));
    p_fence_del->device = device;
    p_fence_del->fence = *p_imm_fence;

    error_t err = deletion_stack_push(p_dstack, p_fence_del, vulkan_sync_fence_deinit);
    if(err.code != 0) {
        vulkan_sync_fence_deinit(p_fence_del);
        return err;
    }

    LOG_INFO("Immediate sync structures initiated");

    return SUCCESS;
}

static void vulkan_sync_semaphore_deinit(void* p_void_sem_del)
{
    LOG_DEBUG("Callback: %s", __func__);

    if(p_void_sem_del == NULL) {
        LOG_ERROR("%s: p_void_sem_del is NULL", __func__);
    }

    // Cast pointer
    semaphore_del_t* p_sem_del = (semaphore_del_t*)p_void_sem_del;

    // NULL check struct fields

    vkDestroySemaphore(p_sem_del->device, p_sem_del->sem, VK_NULL_HANDLE);

    free(p_sem_del);
    p_sem_del = NULL;
    p_void_sem_del = NULL;
}

static void vulkan_sync_fence_deinit(void* p_void_fence_del)
{
    LOG_DEBUG("Callback: %s", __func__);

    if(p_void_fence_del == NULL) {
        LOG_ERROR("%s: p_void_fence_del is NULL", __func__);
    }
    // Cast pointer
    fence_del_t* p_fence_del = (fence_del_t*)p_void_fence_del;

    // NULL check struct fields?

    vkDestroyFence(p_fence_del->device, p_fence_del->fence, VK_NULL_HANDLE);

    free(p_fence_del);
    p_fence_del = NULL;
    p_void_fence_del = NULL;
}

VkSemaphoreSubmitInfo vulkan_sync_get_sem_submit_info(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore)
{
    VkSemaphoreSubmitInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    info.semaphore = semaphore;
    info.stageMask = stage_mask;
    info.deviceIndex = 0; // What does this do?
    info.value = 1;

    return info;
}
