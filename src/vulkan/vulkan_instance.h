#ifndef VULKAN_INSTANCE_H_
#define VULKAN_INSTANCE_H_

#include <stdbool.h>
#include <vulkan/vulkan_core.h>

/**
 * Struct used for deleting a debug messenger.
 */
typedef struct vulkan_instance_debug_msg_del_struct_s {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_msg;
} vulkan_instance_debug_msg_del_struct_t;

/**
 * \brief Initiate a vulkan instance.
 *
 * \param[out] p_instance Pointer to the Vkinstance to be initiated.
 * \return True if successful, else false.
 */
bool vulkan_instance_init(VkInstance* p_instance);

/**
 * \brief Destroy the vulkan instance.
 *
 * \param[in] p_void_instance The vulkan instance to be destroyed.
 */
void vulkan_instance_destroy(void* p_void_instance);

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
bool vulkan_instance_debug_msg_init(VkInstance instance, VkDebugUtilsMessengerEXT* p_debug_messenger);

/**
 * \brief Destroy a debug messenger.
 *
 * Destroy a debug messenger using vkDestroyDebugUtilsMessengerEXT.
 *
 * \param[in] p_void_debug_msg The debug messenger to be destroyed.
 */
void vulkan_instance_debug_msg_destroy(void* p_void_debug_msg_del_struct);
#endif // VULKAN_INSTANCE_H_
