#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#include "logger.h"
#include "vulkan/vulkan_types.h"
#include "vulkan/vulkan_cmd.h"

bool vulkan_cmd_frame_init(VkDevice device, queue_family_data_t* p_queues, frame_data_t* p_frames) {
    if(device == NULL) {
        LOG_ERROR("create_commands: p_engine is NULL");
        return false;
    }

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
            LOG_ERROR("Failed to create frame command pool");
            return false;
        }
        render_cmd_alloc_info.commandPool = p_frames[i].cmd_pool;
        if(vkAllocateCommandBuffers(device, &render_cmd_alloc_info, &p_frames[i].main_cmd_buffer) != VK_SUCCESS) {
            LOG_ERROR("Failed to create frame command pool");
            return false;
        }
    }

    // if(!deletion_stack_push(p_engine->p_main_del_stack, p_engine, vkDestroyCommandPool_wrapper)) {
    //     LOG_ERROR("Failed to queue deletion node");
    //     vkDestroyCommandPool_wrapper(p_engine);
    //     return false;
    // }

    // if(!deletion_stack_push(p_engine->p_main_del_stack, p_engine, vkDestroyCommandPool_array_wrapper)) {
    //     LOG_ERROR("Failed to queue deletion node");
    //     vkDestroyCommandPool_array_wrapper(p_engine);
    //     return false;
    // }

    LOG_INFO("Frame command structures created");
    return true;
}

bool vulkan_cmd_imm_init(
    VkDevice device, const queue_family_data_t* p_queues, VkCommandPool* p_imm_cmd_pool,
    VkCommandBuffer* p_imm_cmd_buff) {
    // Create immediate submit command pool and command buffer
    VkCommandPoolCreateInfo cmd_pool_info = {0};

    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_info.queueFamilyIndex = p_queues->graphics_index;

    if(vkCreateCommandPool(device, &cmd_pool_info, VK_NULL_HANDLE, p_imm_cmd_pool) != VK_SUCCESS) {
        LOG_ERROR("Failed to create immediate command pool");
        return false;
    }

    VkCommandBufferAllocateInfo imm_cmd_alloc_info = {0};

    imm_cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    imm_cmd_alloc_info.commandBufferCount = 1;
    imm_cmd_alloc_info.commandPool = *p_imm_cmd_pool;
    imm_cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    if(vkAllocateCommandBuffers(device, &imm_cmd_alloc_info, p_imm_cmd_buff) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate immediate command buffer");
        return false;
    }

    LOG_INFO("Immidiate command structures created");
    return true;
}

void vulkan_cmd_pool_destroy(void* p_void_cmd_del_struct) {
    LOG_DEBUG("Callback: vulkan_cmd_pool_destroy");

    // Cast pointer
    cmd_pool_del_struct_t* p_cmd_pool_del_struct = (cmd_pool_del_struct_t*)p_void_cmd_del_struct;

    vkDestroyCommandPool(p_cmd_pool_del_struct->device, p_cmd_pool_del_struct->cmd_pool, VK_NULL_HANDLE);

    free(p_cmd_pool_del_struct);

    p_cmd_pool_del_struct = NULL;
    p_void_cmd_del_struct = NULL;
}

// static void vkDestroyCommandPool_wrapper(void* p_engine) {
//     LOG_DEBUG("Callback: vkDestroyCommandPool_wrapper");
//     vkDestroyCommandPool(
//         ((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->imm_cmd_pool, VK_NULL_HANDLE);
// }
//
// static void vkDestroyCommandPool_array_wrapper(void* p_engine) {
//     LOG_DEBUG("Callback: vkDestroyCommandPool_array_wrapper");
//     for(int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
//         LOG_DEBUG("    Destroying frame command pool, index: %d", i);
//         vkDestroyCommandPool(
//             ((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->frames[i].cmd_pool, VK_NULL_HANDLE);
//     }
// }
