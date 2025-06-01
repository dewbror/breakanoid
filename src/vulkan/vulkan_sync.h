#ifndef VULKAN_SYNCH_H_
#define VULKAN_SYNCH_H_

#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "util/deletion_stack.h"
#include "vulkan/vulkan_types.h"



error_t vulkan_sync_frame_init(deletion_stack_t* p_dstack, VkDevice device, frame_data_t* p_frames);

error_t vulkan_sync_imm_init(deletion_stack_t* p_dstack, VkDevice device, VkFence* p_imm_fence);

VkSemaphoreSubmitInfo vulkan_sync_get_sem_submit_info(VkPipelineStageFlags2 stage_mask, VkSemaphore semaphore);

#endif // VULKAN_SYNCH_H_
