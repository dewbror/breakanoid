#ifndef VULKAN_INSTANCE_H_
#define VULKAN_INSTANCE_H_

#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "util/deletion_stack.h"

/**
 * \brief Initiate a vulkan instance.
 *
 * \param[out] p_instance Pointer to the Vkinstance to be initiated.
 * \return True if successful, else false.
 */
error_t vulkan_instance_init(deletion_stack_t* p_dstack, VkInstance* p_instance);

/**
 * \brief Initiate the debug messenger
 *
 * Creates a VkDebugUtilsMessengerEXT object using vkCreateDebugUtilsMessengerEXT. Must be destroyed using
 * vulkan_instance_debug_msg_destroy.
 *
 * If validation layers are not enabled, will return true and set *p_debug_messenger = NULL.
 *
 * \param[in] instance The vulkan instance.
 * \param[out] p_debug_messenger Pointer to the debug messenger to be initiated.
 * \return True if successful, else false.
 */
error_t vulkan_debug_msg_init(deletion_stack_t* p_dstack, VkInstance instance,
    VkDebugUtilsMessengerEXT* p_debug_messenger);

#endif // VULKAN_INSTANCE_H_
