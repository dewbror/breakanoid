#include <stdbool.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#include "logger.h"
#include "vulkan/vulkan_types.h"
#include "vulkan/vulkan_synch.h"

bool vulkan_sync_frame_init(VkDevice device, frame_data_t* p_frames) {
    // if(p_engine == NULL) {
    //     LOG_ERROR("create_sync_structs: p_engine is NULL");
    //     return false;
    // }

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
        if(vkCreateFence(device, &fence_info, VK_NULL_HANDLE, &p_frames[i].render_fence) != VK_SUCCESS) {
            LOG_ERROR("Failed to create render fence");
            return false;
        }
        if(vkCreateSemaphore(device, &sem_info, VK_NULL_HANDLE, &p_frames[i].render_semaphore) != VK_SUCCESS) {
            LOG_ERROR("Failed to create render semaphore");
            return false;
        }
        if(vkCreateSemaphore(device, &sem_info, VK_NULL_HANDLE, &p_frames[i].swapchain_semaphore) != VK_SUCCESS) {
            LOG_ERROR("Failed to create swapchain semahpore");
            return false;
        }
    }

    // if(!deletion_stack_push(p_engine->p_main_del_stack, p_engine, vkDestroyFence_wrapper)) {
    //     LOG_ERROR("Failed to queue deletion node");
    //     vkDestroyFence_wrapper(p_engine);
    //     return false;
    // }

    // if(!deletion_stack_push(p_engine->p_main_del_stack, p_engine, vkDestroyFence_Sem_wrapper)) {
    //     LOG_ERROR("Failed to queue deletion node");
    //     vkDestroyFence_Sem_wrapper(p_engine);
    //     return false;
    // }

    LOG_INFO("Frame sync structures created");
    return true;
}

void vulkan_sync_frame_destroy(void* p_void_vulkan_sync_frame_del_struct) {
    LOG_DEBUG("Callback: vulkan_sync_frame_destory");
    // Cast pointer
    vulkan_sync_frame_del_struct_t* p_frame = (vulkan_sync_frame_del_struct_t*)p_void_vulkan_sync_frame_del_struct;

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

bool vulkan_sync_imm_init(VkDevice device, VkFence* p_imm_fence) {
    VkFenceCreateInfo fence_info = {0};

    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if(vkCreateFence(device, &fence_info, VK_NULL_HANDLE, p_imm_fence) != VK_SUCCESS) {
        LOG_ERROR("Failed to create immediate fence");
        return false;
    }

    LOG_INFO("Immediate sync structures created");
    return true;
}

void vulkan_sync_semaphore_destroy(void* p_void_semaphore_del_struct) {
    LOG_DEBUG("Callback: vulkan_sync_semaphore_destroy");
    // Cast pointer
    semaphore_del_struct_t* p_semaphore_del_struct = (semaphore_del_struct_t*)p_void_semaphore_del_struct;

    vkDestroySemaphore(p_semaphore_del_struct->device, p_semaphore_del_struct->sem, VK_NULL_HANDLE);

    free(p_semaphore_del_struct);
    p_semaphore_del_struct = NULL;
    p_void_semaphore_del_struct = NULL;
}

void vulkan_sync_fence_destroy(void* p_void_fence_del_struct) {
    LOG_DEBUG("Callback: vulkan_sync_fence_destroy");
    // Cast pointer
    fence_del_struct_t* p_fence_del_struct = (fence_del_struct_t*)p_void_fence_del_struct;

    vkDestroyFence(p_fence_del_struct->device, p_fence_del_struct->fence, VK_NULL_HANDLE);

    free(p_fence_del_struct);
    p_fence_del_struct = NULL;
    p_void_fence_del_struct = NULL;
}

// static void vkDestroyFence_wrapper(void* p_engine) {
//     LOG_DEBUG("Callback: vkDestroyFence_wrapper");
//     vkDestroyFence(((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->imm_fence, VK_NULL_HANDLE);
// }
//
// static void vkDestroyFence_Sem_wrapper(void* p_engine) {
//     LOG_DEBUG("Callback: vkDestroyFence_Sem_wrapper");
//
//     for(int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
//         LOG_DEBUG("    Destroying frame sync structs, index: %d", i);
//         vkDestroyFence(
//             ((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->frames[i].render_fence,
//             VK_NULL_HANDLE);
//         vkDestroySemaphore(
//             ((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->frames[i].render_semaphore,
//             VK_NULL_HANDLE);
//         vkDestroySemaphore(
//             ((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->frames[i].swapchain_semaphore,
//             VK_NULL_HANDLE);
//     }
// }
