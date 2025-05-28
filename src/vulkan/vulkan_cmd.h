#ifndef VULKAN_CMD_H_
#define VULKAN_CMD_H_

#include <vulkan/vulkan_core.h>

#include "error/error.h"

#include "vulkan/vulkan_types.h"

typedef struct cmd_pool_del_struct_s {
    VkDevice device;
    VkCommandPool cmd_pool;
} cmd_pool_del_struct_t;

error_t vulkan_cmd_frame_init(VkDevice device, queue_family_data_t* p_queues, frame_data_t* p_frames);

error_t vulkan_cmd_imm_init(
    VkDevice device, const queue_family_data_t* p_queues, VkCommandPool* p_imm_cmd_pool,
    VkCommandBuffer* p_imm_cmd_buff);

void vulkan_cmd_pool_destroy(void* p_void_cmd_pool_del_struct);

#endif // VULKAN_CMD_H_
