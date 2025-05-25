#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

#include "logger.h"
#include "util/strbool.h"
#include "vulkan/vulkan_device.h"

static const uint32_t device_extensions_count = 1;
static const char* const device_extensions[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static const uint32_t instance_layers_count = 0;

#ifdef NDEBUG
static const bool enable_validation_layers = false;
#else
static const bool enable_validation_layers = true;
#endif

/**
 * Check if a physical device is suitable for the application. It checks if the required device extensions are
 * supported, if the required queue families (graphic and present) are available and if the swapchain supports is
 * adequate.
 *
 * \param[in] p_engine Pointer to vulkan_engine.
 * \param[in] device The physical device that is checked for suitablility.
 * \return True if the device is suitable, false if it is not.
 */
static bool is_device_suitable(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDevice physical_device);

/**
 * Check that the device has support for the device extensions listed in device_extensions (which is currently just
 * swapchain support).
 *
 * \param[in] device The physical device which the device extension support is being checked agains.
 * \return True if all required device extensions are supported by the device, false if not.
 */
static bool check_device_extension_support(VkPhysicalDevice physical_device);

bool vulkan_physical_device_init(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDevice* p_physical_device) {
    // NULL check
    if(instance == NULL) {
        LOG_ERROR("physical_device_init: instance is NULL");
        return false;
    }

    if(surface == NULL) {
        LOG_ERROR("physical_device_init: surface is NULL");
        return false;
    }

    uint32_t device_count = 0;

    // Get number of devices with Vulkan support
    vkEnumeratePhysicalDevices(instance, &device_count, VK_NULL_HANDLE);

    // If no devices with Vulkan support, error
    if(device_count == 0) {
        LOG_ERROR("Failed to find GPU with Vulkan support");
        return false;
    }

    // Allocate array holding all supported devices
    VkPhysicalDevice* devices = (VkPhysicalDevice*)malloc(device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    // Check for suitable device in devices
    LOG_DEBUG("Looking for suitable devices:");
    for(uint32_t i = 0; i < device_count; ++i) {
        if(is_device_suitable(instance, surface, devices[i])) {
            *p_physical_device = devices[i];
            // msaa_samples    = get_maxUsable_sample_count(p_engine->physical_device);
            // p_engine->msaa_samples = VK_SAMPLE_COUNT_1_BIT;
            break;
        } else {
        }
    }

    // Free devices array
    free(devices); // NOLINT(bugprone-multi-level-implicit-pointer-conversion)
    devices = NULL;

    // If no suitable device was found, runtime error
    if(p_physical_device == NULL) {
        LOG_ERROR("Failed to find suitable GPU");
        return false;
    }

    // LOG_DEBUG("MSAA samples: %d", p_engine->msaa_samples);
    return true;
}

static bool is_device_suitable(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDevice physical_device) {
    if(instance == NULL) {
        LOG_ERROR("is_device_suitable: instance is NULL");
        return false;
    }
    if(physical_device == NULL) {
        LOG_ERROR("is_device_suitable: physical_device is NULL");
        return false;
    }

    // ALL DEVICE PROP. AND FEAT. QUERY SHOULD BE MOVED TO OWN FUNCTION(S)

    // Query basic device properties
    VkPhysicalDeviceVulkan14Properties properties14 = {0};
    properties14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES;

    VkPhysicalDeviceVulkan13Properties properties13 = {0};
    properties13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
    properties13.pNext = &properties14;

    VkPhysicalDeviceVulkan12Properties properties12 = {0};
    properties12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
    properties12.pNext = &properties13;

    VkPhysicalDeviceVulkan11Properties properties11 = {0};
    properties11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
    properties11.pNext = &properties12;

    VkPhysicalDeviceProperties2 properties2 = {0};
    properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties2.pNext = &properties11;

    vkGetPhysicalDeviceProperties2(physical_device, &properties2);

    LOG_DEBUG("Device name: %s", properties2.properties.deviceName);
    LOG_DEBUG(
        "Device supported Vulkan version: %uv%u.%u.%u", VK_API_VERSION_VARIANT(properties2.properties.apiVersion),
        VK_API_VERSION_MAJOR(properties2.properties.apiVersion),
        VK_API_VERSION_MINOR(properties2.properties.apiVersion),
        VK_API_VERSION_PATCH(properties2.properties.apiVersion));

    // Check that the device must support >1.3
    if(properties2.properties.apiVersion < VK_MAKE_VERSION(1, 3, 0)) {
        LOG_ERROR("Device supported vulkan version must be greater than 1.3");
        return false;
    }

    // Query basic device features
    VkPhysicalDeviceVulkan14Features features14 = {0};
    features14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;

    VkPhysicalDeviceVulkan13Features features13 = {0};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.pNext = &features14;

    VkPhysicalDeviceVulkan12Features features12 = {0};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.pNext = &features13;

    VkPhysicalDeviceVulkan11Features features11 = {0};
    features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    features11.pNext = &features12;

    VkPhysicalDeviceFeatures2 features2 = {0};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &features11;

    LOG_DEBUG("Fetching physical device features with vkGetPhysicalDeviceFeatures2");
    vkGetPhysicalDeviceFeatures2(physical_device, &features2);

    LOG_DEBUG("Device supported features:");
    LOG_DEBUG("    1.0 Sampler anisotropy: %s", strbool(features2.features.samplerAnisotropy));
    LOG_DEBUG("    1.3 dynamic rendering: %s", strbool(features13.dynamicRendering));
    LOG_DEBUG("    1.3 synchronization2: %s", strbool(features13.synchronization2));

    // Check that dynamic rendering is supported
    if(features13.dynamicRendering != VK_TRUE) {
        LOG_ERROR("Dynamic rendering not supported by device: %s", properties2.properties.deviceName);
        return false;
    }

    // Check that synch2 is supported
    if(features13.synchronization2 != VK_TRUE) {
        LOG_ERROR("Synchronization2 not supported by device: %s", properties2.properties.deviceName);
        return false;
    }

    if(features2.features.samplerAnisotropy != VK_TRUE) {
        LOG_ERROR("Sampler anisotropy not supported by device: %s", properties2.properties.deviceName);
        return false;
    }

    // Check that the required queue families (graphics and present) are supported
    queue_family_data_t queue_family_data = {0};
    if(!vulkan_device_get_queue_families(surface, physical_device, &queue_family_data)) {
        LOG_ERROR("Required queue families not supported by device: %s", properties2.properties.deviceName);
        return false;
    }

    // Check if required extensions are supported by device
    if(!check_device_extension_support(physical_device)) {
        LOG_ERROR("Required device extensions not supported by device: %s", properties2.properties.deviceName);
        return false;
    }

    // Check if the device and surface are compatible
    swapchain_support_details_t swapchain_support = {0};
    if(!vulkan_device_get_swapchain_support(surface, physical_device, &swapchain_support)) {
        LOG_ERROR("Swapchain not supported by device: %s", properties2.properties.deviceName);
        return false;
    }

    // Swap chain support is sufficient for this tutorial if there is at least one supported image format and
    // one supported presentation mode given the window surface we have.
    // It is important that we only try to query for swap chain support after verifying that the extension is
    // available!!!

    // These are allocated in query swapchain support and must be freed.
    free(swapchain_support.formats);
    swapchain_support.formats = NULL;
    free(swapchain_support.present_modes);
    swapchain_support.present_modes = NULL;

    LOG_DEBUG("Device %s is suitable", properties2.properties.deviceName);
    return true;
}

bool vulkan_device_get_queue_families(
    VkSurfaceKHR surface, VkPhysicalDevice physical_device, queue_family_data_t* p_queues) {
    if(surface == NULL) {
        LOG_ERROR("vulkan_device_get_queue_families: surface is NULL");
        return false;
    }

    if(physical_device == NULL) {
        LOG_ERROR("vulkan_device_get_queue_families: physical_device is NULL");
        return false;
    }

    // It has been briefly touched upon before that almost every operation in Vulkan, anything from drawing to
    // uploading textures, requires commands to be submitted to a queue. There are different types of queues that
    // originate from different queue families and each family of queues allows only a subset of commands. For
    // example, there could be a queue family that only allows processing of compute commands or one that only
    // allows memory transfer related commands. We need to check which queue families are supported by the device
    // and which one of these supports the commands that we want to use. For that purpose we’ll add a new function
    // findQueueFamilies that looks for all the queue families we need.
    // queue_family_indices q_fam_indices;

    // Query queue family count
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, VK_NULL_HANDLE);

    // Query queue families
    VkQueueFamilyProperties* queue_families =
        (VkQueueFamilyProperties*)malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);

    bool got_graphics = false;
    bool got_present = false;
    for(uint32_t i = 0; i < queue_family_count; ++i) {
        VkBool32 present_support = false;

        // Query if the queue family with index i supports presentation
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
        
        // Check if queue family with index is a graphics queue
        if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            p_queues->graphics_index = i;
            LOG_TRACE("graphics queue family index: %u", i);
            got_graphics = true;
        }

        if(present_support) {
            p_queues->present_index = i;
            LOG_TRACE("present queue family index: %u", i);
            got_present = true;
        }
        
        // When we have found the graphics queue and present queue we are done
        if(got_graphics && got_present) {
            free(queue_families);
            queue_families = NULL;

            return true;
        }
    }

    // Note that it’s very likely that these end up being the same queue family after all, but throughout the
    // program we will treat them as if they were separate queues for a uniform approach. Nevertheless, you could
    // add logic to explicitly prefer a physical device that supports drawing and presentation in the same queue for
    // improved performance.

    free(queue_families);
    queue_families = NULL;

    return false;
}

static bool check_device_extension_support(VkPhysicalDevice physical_device) {
    if(physical_device == NULL) {
        LOG_ERROR("check_device_extension_support: physical_device is NULL");
        return false;
    }

    // Query available device extensions count
    uint32_t available_extensions_count = 0;
    vkEnumerateDeviceExtensionProperties(physical_device, VK_NULL_HANDLE, &available_extensions_count, VK_NULL_HANDLE);

    VkExtensionProperties* available_extensions =
        (VkExtensionProperties*)malloc(available_extensions_count * sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(
        physical_device, VK_NULL_HANDLE, &available_extensions_count, available_extensions);

#ifndef NDEBUG
    LOG_DEBUG("Available device extensions");
    for(uint32_t i = 0; i < available_extensions_count; ++i) {
        LOG_DEBUG("    %s v%u", available_extensions[i].extensionName, available_extensions[i].specVersion);
    }
#endif
    
    // For each device extension in device_extensions, check that it exists in available_extensions.
    for(uint32_t i = 0; i < device_extensions_count; ++i) {
        bool req_ext_found = false;
        for(uint32_t j = 0; j < available_extensions_count; ++j) {
            if(strcmp(device_extensions[i], available_extensions[j].extensionName) == 0) {
                req_ext_found = true;
                break;
            }
        }

        // If any required extension is not available then fail.
        if(!req_ext_found) {
            return false;
        }
    }

    free(available_extensions);
    available_extensions = NULL;

    return true;
}

bool vulkan_device_get_swapchain_support(
    VkSurfaceKHR surface, VkPhysicalDevice physical_device, swapchain_support_details_t* p_details) {
    if(surface == NULL) {
        LOG_ERROR("vulkan_device_get_swapchain_support: surface is NULL");
        return false;
    }

    if(physical_device == NULL) {
        LOG_ERROR("vulkan_device_get_swapchain_support: physical_device is NULL");
        return false;
    }

    // Just checking if a swap chain is available is not sufficient, because it may not actually be compatible with
    // our window surface. Creating a swap chain also involves a lot more settings than instance and device
    // creation, so we need to query for some more details before we’re able to proceed.
    // swapchain_support_details details;

    // Query basic surface capabilities.
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &p_details->capabilities);

    // Query supported surface formats.
    uint32_t formats_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formats_count, VK_NULL_HANDLE);
    if(formats_count != 0) {
        p_details->formats = (VkSurfaceFormatKHR*)malloc(formats_count * sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formats_count, p_details->formats);
        p_details->formats_count = formats_count;
    } else {
        LOG_ERROR("Device swapchain surface formats unsupported");
        return false;
    }

    // Query supported presentation modes.
    uint32_t present_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_modes_count, VK_NULL_HANDLE);
    if(present_modes_count != 0) {
        p_details->present_modes = (VkPresentModeKHR*)malloc(present_modes_count * sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physical_device, surface, &present_modes_count, p_details->present_modes);
        p_details->present_modes_count = present_modes_count;
    } else {
        LOG_ERROR("Device swapchin surface present modes unsupported");
        return false;
    }

    LOG_DEBUG("Device swapchain supported");
    return true;
}

bool vulkan_device_init(
    VkSurfaceKHR surface, VkPhysicalDevice physical_device, VkDevice* p_device, queue_family_data_t* p_queues) {
    if(surface == NULL) {
       LOG_ERROR("vulkan_device_init: surface is NULL");
       return false;
    }

    if(physical_device == NULL) {
        LOG_ERROR("vulkan_device_init: physical_device is NULL");
        return false;
    }

    // Check device graphics and present queue family support and store their indices p_queues
    if(!vulkan_device_get_queue_families(surface, physical_device, p_queues)) {
        LOG_ERROR("Required queue families not supported by device");
        return false;
    }

    // Hard coded, make dynamic in future?
    uint32_t unique_q_fams_count = 0;
    if(p_queues->graphics == p_queues->present) {
        // The graphics and present queue families are the same, this is the most common situation
        unique_q_fams_count = 1;
    } else {
        // They are not the same, not common but we will support it?
        unique_q_fams_count = 2;
    }
    LOG_DEBUG("Unique queue families:  %u", unique_q_fams_count);

    // Allocate array of ints to hold the queue family indices
    uint32_t* unique_q_fams = (uint32_t*)malloc(unique_q_fams_count * sizeof(uint32_t));
    if(unique_q_fams == NULL) {
        LOG_ERROR("Failed to allocate memory of size %lu: %s", unique_q_fams_count * sizeof(uint32_t), strerror(errno));
    }

    // Fill allocated arrat with the queue family indices 
    if(unique_q_fams_count == 1) {
        unique_q_fams[0] = p_queues->graphics_index;
    } else if(unique_q_fams_count == 2) {
        unique_q_fams[0] = p_queues->graphics_index;
        unique_q_fams[1] = p_queues->present_index;
    } else {
        LOG_ERROR("More than two queue families is unsupported currently");
        return false;
    }

    // Create array of device queue create infos
    VkDeviceQueueCreateInfo* q_create_infos =
        (VkDeviceQueueCreateInfo*)malloc(unique_q_fams_count * sizeof(VkDeviceQueueCreateInfo));
    if(q_create_infos == NULL) {
        LOG_ERROR(
            "Failed to allocate memory of size %lu: %s", unique_q_fams_count * sizeof(VkDeviceQueueCreateInfo),
            strerror(errno));
        return false;
    }

    // Fill our device queue create info(s)
    float q_priority = 1.0f;
    for(uint32_t i = 0; i < unique_q_fams_count; ++i) {
        VkDeviceQueueCreateInfo q_create_info = {0};
        q_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q_create_info.queueFamilyIndex = unique_q_fams[i]; // <-- Which queue family do we want
        q_create_info.queueCount = 1; // <-- How many queues of this queue family do we want to create?
        q_create_info.pQueuePriorities = &q_priority;
        q_create_infos[i] = q_create_info;
    }

    // Enable device features (non currently)
    VkPhysicalDeviceVulkan13Features features13 = {0};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    VkPhysicalDeviceVulkan12Features features12 = {0};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.pNext = &features13;

    VkPhysicalDeviceVulkan11Features features11 = {0};
    features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    features11.pNext = &features12;

    VkPhysicalDeviceFeatures2 features2 = {0};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &features11;
    
    // Enable the features we want
    features2.features.samplerAnisotropy = VK_TRUE;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    // Start filling the main VkDeviceCreateInfo structure.
    VkDeviceCreateInfo create_dev_info = {0};
    create_dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_dev_info.pQueueCreateInfos = q_create_infos;
    create_dev_info.queueCreateInfoCount = unique_q_fams_count;

    create_dev_info.pNext = &features2;

    // Enabeling device extensions, like swapchain
    create_dev_info.enabledExtensionCount = device_extensions_count;
    create_dev_info.ppEnabledExtensionNames = device_extensions;

    if(vkCreateDevice(physical_device, &create_dev_info, VK_NULL_HANDLE, p_device) != VK_SUCCESS) {
        LOG_ERROR("Failed to create vulkan logical device");
        return false;
    }
    LOG_INFO("Vulkan logical device created");

    // The queues are automatically created along with the logical device.
    // We can use the vkGetDeviceQueue function to retrieve queue handles for each queue family. The parameters are
    // the logical device, queue family, queue index and a pointer to the variable to store the queue handle in.
    // Because we’re only creating a single queue from "each" family, we’ll simply use index 0.
    vkGetDeviceQueue(*p_device, p_queues->graphics_index, 0, &p_queues->graphics);
    vkGetDeviceQueue(*p_device, p_queues->present_index, 0, &p_queues->present);

    // Free dynamically allocated arrays
    free(unique_q_fams);
    unique_q_fams = NULL;
    free(q_create_infos);
    q_create_infos = NULL;

    return true;
}

void vulkan_device_destroy(void* p_void_device) {
    LOG_DEBUG("Callback: vulkan_device_destroy");

    // NULL check
    if(p_void_device == NULL) {
        LOG_ERROR("vulkan_device_destroy: device is NULL");
        return;
    }

    // Cast pointer
    VkDevice device = (VkDevice)p_void_device;

    // Destroy device
    vkDestroyDevice(device, VK_NULL_HANDLE);
}
