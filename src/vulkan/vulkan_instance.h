#ifndef VULKAN_INSTANCE_H_
#define VULKAN_INSTANCE_H_

#include <stdbool.h>
#include <vulkan/vulkan_core.h>

/**
 *
 */
typedef struct vulkan_instance_debug_msg_del_struct_s {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_msg;
} vulkan_instance_debug_msg_del_struct_t;

/**
 * Initiate a vulkan instance.
 *
 * \param[in] instance The VkInstance to be initiated.
 *
 * \return True if successful, false if failed.
 */
bool vulkan_instance_init(VkInstance* p_instance);

/**
 *
 */
void vulkan_instance_destroy(void* p_del_resource);

/**
 * If enable_validation_layers = true, will populate a VkDebugUtilsMessengerCreateInfoEXT using
 * populate_debug_messenger_create_info and create a VkDebugUtilsMessengerEXT object using
 * vkCreateDebugUtilsMessengerEXT. The VkDebugUtilsMessengerCreateInfoEXT must be destroyed using
 * DestroyDebugUtilsMessengerEXT.
 *
 * \param[in] p_engine Pointer to the vulkan_engine.
 *
 * \return True if successful, false if failed.
 */
bool vulkan_instance_debug_msg_init(VkInstance instance, VkDebugUtilsMessengerEXT* p_debug_messenger);

/**
 *
 */
void vulkan_instance_debug_msg_destroy(void* p_del_struct);
#endif // VULKAN_INSTANCE_H_
