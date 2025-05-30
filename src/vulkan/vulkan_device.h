#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include <stdbool.h>
#include <vulkan/vulkan_core.h>

#include "error/error.h"

#include "vulkan/vulkan_types.h"

/**
 * /brief Initiate a physical device.
 *
 * \param[in] instance The vulkan instance.
 * \param[in] surface The vulkan rendering surface.
 * \param[out] p_physical_device Pointer to the physical device to be initiated.
 * \return True if successful, else false.
 */
error_t vulkan_physical_device_init(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDevice* p_physical_device);

/**
 * \brief Check and get device swapchain support.
 *
 * Check if the devices swapchain has the required support (which is just that it has non-zero formats and non-zero
 * present modes). It also stores the formats and present modes in the swapchain_support_details struct.
 *
 * \param[in] p_engine Pointer to the vulkna_engine.
 * \param[in] device The physical device being checked for swapchain support.
 * \param[out] swapchain_support Pointer to a swapchain_support_details_t which stores the swapchain formats and
 * present modes.
 * \return True if the swapchain has support, false if not.
 *
 * \warning This funciton dynamically allocates arrays in it which must be freed!
 */
bool vulkan_device_get_swapchain_support(VkSurfaceKHR surface, VkPhysicalDevice physical_device,
    swapchain_support_details_t* swapchain_support);

/**
 * \brief Check and get queue family indices.
 *
 * Check for and fetches the graphics queue family index and the present queue family index and stores them in a
 * queue_family_data_t.
 *
 * \param[in] p_engine Pointer to the vulkan_engine.
 * \param[in] device The physical device from wich the queuf family indeices are fetched from.
 * \param[out] p_queues Pointer to a queue_family_data_t which stores the indices if they are found.
 * \return True if both a grapics queue family and a present queue family was found.
 */
bool vulkan_device_get_queue_families(VkSurfaceKHR surface, VkPhysicalDevice physical_device,
    queue_family_data_t* p_queues);

/**
 * \brief Initiate vulkan logical device.
 *
 * Initiated a vulkan logical device and also fetches the queue information.
 *
 * \param[in] surface The vulkan render surface.
 * \param[in] physical_device The physical device.
 * \param[out] p_device Pointer to the vulkan device to be initiated.
 * \param[out] p_queues Pointer to the queue_family_data_t which will hold the queue information.
 * \return True if successful, else false.
 */
error_t vulkan_device_init(VkSurfaceKHR surface, VkPhysicalDevice physical_device, VkDevice* p_device,
    queue_family_data_t* p_queue_family_data);
/**
 * \brief Destroy a vulkan device.
 *
 * \param[in] p_void_device The vulkan device to be destroyed.
 */
void vulkan_device_destroy(void* p_void_device);

#endif // VULKAN_DEVICE_H_
