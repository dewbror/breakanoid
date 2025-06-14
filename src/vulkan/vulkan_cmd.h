#ifndef VULKAN_CMD_H_
#define VULKAN_CMD_H_

#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "util/deletion_stack.h"
#include "vulkan/vulkan_types.h"

/**
 * Initiate the command pool and buffer in the frame_data_t.
 */
error_t vulkan_cmd_frame_init(deletion_stack_t* p_dstack, VkDevice device, const queue_family_data_t* p_queues, frame_data_t* p_frames);

/**
 * Initiate the immediate command pool and command buffer
 */
error_t vulkan_cmd_imm_init(deletion_stack_t* p_dstack, VkDevice device, const queue_family_data_t* p_queues, VkCommandPool* p_imm_cmd_pool,
    VkCommandBuffer* p_imm_cmd_buff);

/**
 * Get a VkCommandBufferSubmitInfo.
 */
VkCommandBufferSubmitInfo vulkan_cmd_get_buffer_submit_info(VkCommandBuffer cmd);

/**
 * Get a VkSubmitInfo2.
 */
VkSubmitInfo2 vulkan_cmd_get_submit_info2(const VkCommandBufferSubmitInfo* cmd_buffer_submit_info,
    const VkSemaphoreSubmitInfo* signal_semaphore_submit_info, const VkSemaphoreSubmitInfo* wait_semaphore_submit_info);

#endif // VULKAN_CMD_H_
