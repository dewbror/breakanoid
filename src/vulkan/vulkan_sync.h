#ifndef VULKAN_SYNCH_H_
#define VULKAN_SYNCH_H_

#include <stdbool.h>
#include <vulkan/vulkan_core.h>

#include "vulkan/vulkan_types.h"

typedef struct semaphore_del_struct_s {
    VkDevice device;
    VkSemaphore sem;
} semaphore_del_struct_t;

typedef struct fence_del_struct_s {
    VkDevice device;
    VkFence fence;
} fence_del_struct_t;

typedef struct vulkan_sync_frame_del_struct_s {
    VkDevice device;
    frame_data_t* p_frames;
} vulkan_sync_frame_del_struct_t;

bool vulkan_sync_frame_init(VkDevice device, frame_data_t* p_frames);

void vulkan_sync_frame_destroy(void* p_void_vulkan_sync_frame_del_struct);

bool vulkan_sync_imm_init(VkDevice device, VkFence* p_imm_fence);

void vulkan_sync_semaphore_destroy(void* p_void_semaphore_del_struct);

void vulkan_sync_fence_destroy(void* p_void_fence_del_struct);

#endif // VULKAN_SYNCH_H_
