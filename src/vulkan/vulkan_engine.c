#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_vulkan.h>

#include "logger.h"
#include "vulkan/vulkan_engine.h"
#include "vulkan/vulkan_instance.h"
#include "util/deletion_queue.h"
#include "SDL/SDL_backend.h"

#define HEIGHT 1080
#define WIDTH  1920

static const uint32_t device_extensions_count = 2;
static const char* const device_extensions[2] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                 VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME};

static const uint32_t instance_layers_count = 0;

#ifdef NDEBUG
static const bool enable_validation_layers = false;
#else
static const bool enable_validation_layers = true;
#endif

/**
 * Struct for holding the graphics queue family index and the present queue family index.
 */
typedef struct queue_family_indices_s {
    uint32_t graphics_family;
    uint32_t present_family;
} queue_family_indices_t;

/**
 * Struct used for finding the swapchain support details.
 */
typedef struct swapchain_support_details_s {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR* formats;
    size_t formats_count;
    VkPresentModeKHR* present_modes;
    size_t present_modes_count;
} swapchain_support_details_t;

/**
 * vkDestroySurfaceKHR wrapper for use in deletion queue
 *
 * \param[in] p_engine Pointer to vulkan engine.
 */
static void vkDestroySurfaceKHR_wrapper(void* p_engine);

/**
 * Pick a physical device.
 *
 * \param[in] p_engine Pointer to vulkan_engine.
 *
 * \return True if successful, false if failed.
 */
static bool pick_physical_device(vulkan_engine_t* p_engine);

/**
 * Check if a physical device is suitable for the application. It checks if the required device extensions are
 * supported, if the required queue families (graphic and present) are available and if the swapchain supports is
 * adequate.
 *
 * \param[in] p_engine Pointer to vulkan_engine.
 * \param[in] device The physical device that is checked for suitablility.
 *
 * \return True if the device is suitable, false if it is not.
 */
static bool is_device_suitable(vulkan_engine_t* p_engine, VkPhysicalDevice device);

/**
 * Check for and fetches the graphics queue family index and the present queue family index and stores them in a
 * swapchain_support_details struct.
 *
 * \param[in] p_engine Pointer to the vulkan_engine.
 * \param[in] device The physical device from wich the queuf family indeices are fetched from.
 * \param[out] q_fam_indices Pointer to a queue_family_indices struct which stores the indices if they are found.
 *
 * \return True if both a grapics queue family and a present queue family was found.
 */
static bool get_queue_families(vulkan_engine_t* p_engine, VkPhysicalDevice device,
                               queue_family_indices_t* q_fam_indices);

/**
 * Check that the device has support for the device extensions listed in device_extensions (which is currently just
 * swapchain support).
 *
 * \param[in] device The physical device which the device extension support is being checked agains.
 *
 * \return True if all required device extensions are supported by the device, false if not.
 */
static bool check_device_extension_support(VkPhysicalDevice device);

/**
 * Check if the devices swapchain has the required support (which is just that it has non-zero formats and non-zero
 * present modes). It also stores the formats and present modes in the swapchain_support_details struct. This struct has
 * dynamically allocated arrays in it and must be freed at some point after calling this funciton.
 *
 * \param[in] p_engine Pointer to the vulkna_engine.
 * \param[in] device The physical device being checked for swapchain support.
 * \param[out] swapchain_support Pointer to a swapchain_support_details struct which stores the swapchain formats and
 * present modes.
 *
 * \return True if the swapchain has support, false if not.
 */
static bool get_swapchain_support(vulkan_engine_t* p_engine, VkPhysicalDevice device,
                                  swapchain_support_details_t* swapchain_support);

/**
 * free wrapper for use in deletion queue.
 *
 * \param[in] mem Pointer to the memory to be freed.
 */
static void free_wrapper(void* mem);

/**
 * Create a logical device. Also fetches the queues.
 *
 * \param[in] p_engine Pointer to the vulkan_engine.
 *
 * \return True if successful, false if failed.
 */
static bool create_logical_device(vulkan_engine_t* p_engine);

/**
 * vkDestroyDevice wrapper for use in deletion queue
 *
 * \param[in] p_resource Pointer to the resource to be destroyed or a struct with a field that is to be destroyed.
 */
static void vkDestroyDevice_wrapper(void* p_resource);

/**
 * Create a swapchain and getting the swapchain images.
 *
 * \param[in] p_engine Pointer to the vulkan_engine.
 */
static bool create_swapchain(vulkan_engine_t* p_engine);

/**
 * Choose the swapchain surface format.
 *
 * \param[in] p_formats Array of supported swapchain surface formats.
 * \param[in] formats_count Number of elements in the formats array.
 *
 * \return The choosen swapchain surface format.
 */
static VkSurfaceFormatKHR choose_swapchain_surface_format(VkSurfaceFormatKHR* p_formats, size_t formats_count);

/**
 * Choose the swapchain present mode.
 *
 * \param[in] p_present_modes Array of supported swapchain present modes.
 * \param[in] present_modes_count Number of elements in the present modes array.
 *
 * \return The chosen present mode.
 */
static VkPresentModeKHR choose_swapchain_present_mode(VkPresentModeKHR* p_present_modes, size_t present_modes_count);

/**
 * Choose the swapchain extent. This is chosen to the pixel size given by SDL_GetWindowSizeInPixels. This is fairly
 * straight forward except for when the display is a "high pixel density display" in which case the pixel size can be
 * larger than the screen coordinates. SDL_GetWindowSizeInPixels should get the actual pixel size and not the screen
 * coordiantes.
 *
 * \param[in] p_engine Pointer to the vulkan_engine.
 * \param[in] capabilities The swapchain surface capabilities.
 *
 * \return The chosen swapchain extent. If the SDL_GetWindowSizeInPixels the extent will have 0 width and height.
 */
static VkExtent2D choose_swapchain_extent(vulkan_engine_t* p_engine, VkSurfaceCapabilitiesKHR capabilities);

/**
 * vkDestroySwapchainKHR for use in deletion queue.
 *
 * \param[in] p_engine Pointer to the vulkan engine
 */
static void vkDestroySwapchainKHR_wrapper(void* p_engine);

/**
 * Creating image views for the swapchain images.
 *
 * \param[in] p_engine Pointer to the vulkna engine.
 *
 * \return True if all image views were successfully created, false otherwise.
 */
static bool create_image_views(vulkan_engine_t* p_engine);

/**
 * Create and image view for an image.
 *
 * \param[in] image The image for which the image view is created.
 * \param[in] format The format of the image view.
 * \param[in] aspect_flags
 * \param[in] mip_levels
 *
 * \return The image view.
 */
static bool get_image_view(vulkan_engine_t* p_engine, VkImageView image_view, VkImage image, VkFormat format,
                           VkImageAspectFlags aspect_flags, uint32_t mip_levels);
/**
 * vkDestroyImageView wrapper for swapchin image view array
 *
 * \param[in] p_engine Pointer to the vulkan engine
 */
static void vkDestroyImageView_array_wrapper(void* p_engine);

/**
 * vkDestroyImage and vkFreeMemory wrapper allocated images
 *
 * \param[in] p_engine Pointer to the vulkan engine
 */
static void vkDestroyImage_FreeMemory_wrapper(void* p_engine);

/**
 * vkDestroyImageView wrapper
 *
 * \param[in] p_engine Pointer to the vulkan engine
 */
static void vkDestroyImageView_wrapper(void* p_engine);

/**
 * Create the command pools/allocate command buffers for commands submitted to graphics queue and for immediate submits.
 *
 * \param[in] p_engine Pointer to the vulkan engine
 */
static bool create_commands(vulkan_engine_t* p_engine);

/**
 * vkDestroyCommandPool wrapper
 *
 * \param[in] p_engine Pointer to the vulkan engine
 */
static void vkDestroyCommandPool_wrapper(void* p_engine);

/**
 * vkDestroyCommandPool wrapper for command pools in frame array
 *
 * \param[in] p_engine Pointer to the vulkan engine
 */
static void vkDestroyCommandPool_array_wrapper(void* p_engine);

bool vulkan_engine_init(vulkan_engine_t* p_engine) {
    // Check if p_engine is NULL
    if(p_engine == NULL) {
        LOG_ERROR("vulkan_engine_init: p_engine is NULL");
        return false;
    }

    // Set window extent, this will be performed from some settings file in the future.
    p_engine->window_extent.height = HEIGHT;
    p_engine->window_extent.width  = WIDTH;

    // Allocate main deletion queue
    p_engine->p_main_delq = deletion_queue_alloc();
    // This NULL check is technically unnecessary since it is also performed inside deletion_queue_alloc
    if(p_engine->p_main_delq == NULL) {
        LOG_ERROR("Vulkan engine deletion queue is NULL after allocation");
        return false;
    }

    // Initialize SDL
    if(!init_SDL_backend(p_engine)) {
        LOG_ERROR("Failed to initialize SDL backend");
        return false;
    }

    // Create vulkan instance
    if(!vulkan_instance_init(&p_engine->instance)) {
        LOG_ERROR("Failed to create vulkan instance");
        return false;
    }

    if(!deletion_queue_queue(p_engine->p_main_delq, p_engine->instance, vulkan_instance_destroy)) {
        LOG_ERROR("Failed to queue deletion node");
        vulkan_instance_destroy(p_engine->instance);
        return false;
    }

    // Setup debug messenger
    if(!vulkan_instance_debug_msg_init(p_engine->instance, &p_engine->debug_msg)) {
        LOG_ERROR("Failed to setup debug messenger");
        return false;
    }

    vulkan_instance_debug_msg_del_struct_t* p_debug_msg_del_struct =
        (vulkan_instance_debug_msg_del_struct_t*)malloc(sizeof(vulkan_instance_debug_msg_del_struct_t));

    p_debug_msg_del_struct->instance  = p_engine->instance;
    p_debug_msg_del_struct->debug_msg = p_engine->debug_msg;

    if(!deletion_queue_queue(p_engine->p_main_delq, p_debug_msg_del_struct, vulkan_instance_debug_msg_destroy)) {
        LOG_ERROR("Failed to queue deletion callback");
        vulkan_instance_debug_msg_destroy(p_debug_msg_del_struct);
        return false;
    }

    // Create SDL window surface
    if(!SDL_Vulkan_CreateSurface(p_engine->p_SDL_window, p_engine->instance, VK_NULL_HANDLE, &p_engine->surface)) {
        LOG_ERROR("Failed to create vulkan rendering surface: %s", SDL_GetError());
        return false;
    }
    LOG_INFO("Vulkan rendering surface created");
    if(!deletion_queue_queue(p_engine->p_main_delq, p_engine, vkDestroySurfaceKHR_wrapper)) {
        LOG_ERROR("Failed to queue deletion node");
        return false;
    }

    // Pick a physical device (GPU)
    if(!pick_physical_device(p_engine)) {
        LOG_ERROR("Failed to find suitible physical device");
        return false;
    }

    // Create logical device and get queue families
    if(!create_logical_device(p_engine)) {
        LOG_ERROR("Failed to create logical device");
        return false;
    }

    // Create swapchain
    if(!create_swapchain(p_engine)) {
        LOG_ERROR("Failed to create swapchain");
        return false;
    }

    // Create image views
    if(!create_image_views(p_engine)) {
        LOG_ERROR("Failed to create swapchain image views\n");
        return false;
    }

    // Create commands
    if(!create_commands(p_engine)) {
        LOG_ERROR("Failed to create commands");
        return false;
    }

    // Just playing around with cglm
    // vec3 vectorX = {1.0f, .0f, .0f};
    // vec3 vectorZ = {.0f, .0f, 1.0f};
    // mat4 matrix;
    // glm_mat4_identity(matrix);
    // glm_rotate(matrix, glm_rad(90.0f), vectorZ);
    // vec3 dest;
    // glm_vec3_rotate_m4(matrix, vectorX, dest);
    // printf("Before rot:\t   x = %.2f, y = %.2f, z = %.2f\n", vectorX[0], vectorX[1], vectorX[2]);
    // printf("After rot:\t    x = %.2f, y = %.2f, z = %.2f\n", dest[0], dest[1], dest[2]);

    LOG_INFO("Vulkan engine initialized");
    return true;
}

bool vulkan_engine_destroy(vulkan_engine_t* p_engine) {
    // Check if p_engine is NULL
    if(p_engine == NULL) {
        LOG_ERROR("vulkan_engine_destroy: p_engine is NULL");
        return false;
    }

    // Flush deletion queue
    if(!deletion_queue_flush(&p_engine->p_main_delq)) {
        LOG_ERROR("Failed to flush deletion queue");
        return false;
    }

    // Check that p_main_delq is NULL after flushing
    if(p_engine->p_main_delq != NULL) {
        LOG_ERROR("Failed to flush deletion queue");
        return false;
    }

    // Vulkan engine successfully destroyed
    LOG_INFO("Vulkan engine destroyed");
    return true;
}

static void vkDestroySurfaceKHR_wrapper(void* p_engine) {
    LOG_DEBUG("Callback: vkDestroySurfaceKHR_wrapper");
    vkDestroySurfaceKHR(((vulkan_engine_t*)p_engine)->instance, ((vulkan_engine_t*)p_engine)->surface, VK_NULL_HANDLE);
}

static bool pick_physical_device(vulkan_engine_t* p_engine) {
    if(p_engine == NULL) {
        LOG_ERROR("pick_physical_device: p_engine is NULL");
        return false;
    }

    uint32_t device_count = 0;

    // Get number of devices with Vulkan support
    vkEnumeratePhysicalDevices(p_engine->instance, &device_count, VK_NULL_HANDLE);

    // If no devices with Vulkan support, error
    if(device_count == 0) {
        LOG_ERROR("Failed to find GPU with Vulkan support");
        return false;
    }

    // Allocate array holding all supported devices
    VkPhysicalDevice* devices = (VkPhysicalDevice*)malloc(device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(p_engine->instance, &device_count, devices);

    // Check for suitable device in devices
    LOG_DEBUG("Looking for suitable devices:");
    for(uint32_t i = 0; i < device_count; ++i) {
        if(is_device_suitable(p_engine, devices[i])) {
            p_engine->physical_device = devices[i];
            // msaa_samples    = get_maxUsable_sample_count(p_engine->physical_device);
            p_engine->msaa_samples = VK_SAMPLE_COUNT_1_BIT;
            break;
        }
    }

    // Free devices array
    free((void*)devices);

    // If no suitable device was found, runtime error
    if(p_engine->physical_device == NULL) {
        LOG_ERROR("Failed to find suitable GPU");
        return false;
    }
    LOG_DEBUG("MSAA samples: %d", p_engine->msaa_samples);
    return true;
}

static bool is_device_suitable(vulkan_engine_t* p_engine, VkPhysicalDevice device) {
    if(p_engine == NULL) {
        LOG_ERROR("is_device_suitable: p_engine is NULL");
        return false;
    }
    if(device == NULL) {
        LOG_ERROR("is_device_suitable: device is NULL");
        return false;
    }

    // Query basic device properties like name and supported Vulkan version
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    // The support for optional features like texture compression, 64 bit floats and multi viewport rendering
    // (useful for VR) can be queried using vkGetPhysicalDeviceFeatures:
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(device, &device_features);

    LOG_DEBUG("Device name: %s", device_properties.deviceName);
    LOG_DEBUG("Device supported Vulkan version: %u.%u.%u.%u", VK_API_VERSION_VARIANT(device_properties.apiVersion),
              VK_API_VERSION_MAJOR(device_properties.apiVersion), VK_API_VERSION_MINOR(device_properties.apiVersion),
              VK_API_VERSION_PATCH(device_properties.apiVersion));

    // Check if graphics and present queue families are supported by device
    queue_family_indices_t q_fam_indices = {0};
    if(!get_queue_families(p_engine, device, &q_fam_indices)) {
        LOG_ERROR("Required queue families not supported by device: %s", device_properties.deviceName);
        return false;
    }

    // Check if required extensions are supported by device
    if(!check_device_extension_support(device)) {
        LOG_ERROR("Required device extensions not supported by device: %s", device_properties.deviceName);
        return false;
    }

    // Check if the device and surface are compatible
    swapchain_support_details_t swapchain_support = {0};
    bool swapchain_adequate                       = get_swapchain_support(p_engine, device, &swapchain_support);
    // Swap chain support is sufficient for this tutorial if there is at least one supported image format and
    // one supported presentation mode given the window surface we have.
    // It is important that we only try to query for swap chain support after verifying that the extension is
    // available!!!

    // These are allocated in query swapchain support and must be freed.
    free(swapchain_support.formats);
    free(swapchain_support.present_modes);

    VkPhysicalDeviceFeatures supported_features;
    vkGetPhysicalDeviceFeatures(device, &supported_features);

    if(swapchain_adequate && supported_features.samplerAnisotropy) {
        LOG_INFO("Physical device picked: %s", device_properties.deviceName);
        return true;
    } else {
        if(!swapchain_adequate) {
            // TODO: Move LOG_ERROR to inside query_swapchain_support
            LOG_ERROR("Swapchain not supported by device: %s", device_properties.deviceName);
        }
        if(!supported_features.samplerAnisotropy) {
            LOG_ERROR("Sampler anisotropy not supported by device: %s", device_properties.deviceName);
        }
        return false;
    }
}

static bool get_queue_families(vulkan_engine_t* p_engine, VkPhysicalDevice device,
                               queue_family_indices_t* q_fam_indices) {
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
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, VK_NULL_HANDLE);

    // Query queue families
    VkQueueFamilyProperties* queue_families =
        (VkQueueFamilyProperties*)malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    bool got_graphics = false, got_present = false;
    for(uint32_t i = 0; i < queue_family_count; ++i) {
        VkBool32 present_support = false;

        // Check if the device has support for presentation queue
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, p_engine->surface, &present_support);

        if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            q_fam_indices->graphics_family = i;
            LOG_TRACE("graphics queue family index: %u", i);
            got_graphics = true;
        }
        if(present_support) {
            q_fam_indices->present_family = i;
            LOG_TRACE("present queue family index: %u", i);
            got_present = true;
        }
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

static bool check_device_extension_support(VkPhysicalDevice device) {
    if(device == NULL) {
        LOG_ERROR("check_device_extension_support: device is NULL");
        return false;
    }

    // Query available device extensions count
    uint32_t available_extensions_count = 0;
    vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &available_extensions_count, VK_NULL_HANDLE);

    VkExtensionProperties* available_extensions =
        (VkExtensionProperties*)malloc(available_extensions_count * sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &available_extensions_count, available_extensions);

#ifndef NDEBUG
    LOG_DEBUG("Available device extensions");
    for(uint32_t i = 0; i < available_extensions_count; ++i) {
        LOG_DEBUG("    %s v%u", available_extensions[i].extensionName, available_extensions[i].specVersion);
    }
#endif

    for(uint32_t i = 0; i < device_extensions_count; ++i) {
        bool req_ext_found = false;
        for(uint32_t j = 0; j < available_extensions_count; ++j) {
            if(strcmp(device_extensions[i], available_extensions[j].extensionName) == 0) {
                req_ext_found = true;
                break;
            }
        }
        if(!req_ext_found) {
            return false;
        }
    }

    free(available_extensions);
    return true;
}

static bool get_swapchain_support(vulkan_engine_t* p_engine, VkPhysicalDevice device,
                                  swapchain_support_details_t* p_details) {
    // Just checking if a swap chain is available is not sufficient, because it may not actually be compatible with
    // our window surface. Creating a swap chain also involves a lot more settings than instance and device
    // creation, so we need to query for some more details before we’re able to proceed.
    // swapchain_support_details details;

    // Query basic surface capabilities.
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, p_engine->surface, &p_details->capabilities);

    // Query supported surface formats.
    uint32_t formats_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, p_engine->surface, &formats_count, VK_NULL_HANDLE);
    if(formats_count != 0) {
        p_details->formats = (VkSurfaceFormatKHR*)malloc(formats_count * sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, p_engine->surface, &formats_count, p_details->formats);
        p_details->formats_count = formats_count;
        // deletion_queue_queue(p_engine->p_main_delq, details->formats, free_wrapper);
    } else {
        LOG_ERROR("Device swapchain surface formats unsupported");
        return false;
    }

    // Query supported presentation modes.
    uint32_t present_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, p_engine->surface, &present_modes_count, VK_NULL_HANDLE);
    if(present_modes_count != 0) {
        p_details->present_modes = (VkPresentModeKHR*)malloc(present_modes_count * sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, p_engine->surface, &present_modes_count,
                                                  p_details->present_modes);
        p_details->present_modes_count = present_modes_count;
        // deletion_queue_queue(p_engine->p_main_delq, details->present_modes, free_wrapper);
    } else {
        LOG_ERROR("Device swapchin surface present modes unsupported");
        return false;
    }

    LOG_DEBUG("Device swapchain supported");
    return true;
}

static void free_wrapper(void* p_mem) {
    LOG_DEBUG("Callback: free_wrapper");
    free(p_mem);
}

static bool create_logical_device(vulkan_engine_t* p_engine) {
    if(p_engine == NULL) {
        LOG_ERROR("create_logical_device: p_engine is NULL");
        return false;
    }

    // Check device graphics and present queue family support and store their indices in q_fam_indices
    queue_family_indices_t q_fam_indices = {0};
    if(!get_queue_families(p_engine, p_engine->physical_device, &q_fam_indices)) {
        LOG_ERROR("Required queue families not supported by device");
        return false;
    }

    // Hard coded, make dynamic in future?
    uint32_t unique_q_fams_count = 0;
    if(q_fam_indices.graphics_family == q_fam_indices.present_family) {
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
    if(unique_q_fams_count == 1) {
        unique_q_fams[0] = q_fam_indices.graphics_family;
    } else if(unique_q_fams_count == 2) {
        unique_q_fams[0] = q_fam_indices.graphics_family;
        unique_q_fams[1] = q_fam_indices.present_family;
    } else {
        LOG_ERROR("More than two queue families is unsupported currently");
        return false;
    }

    // Create array of device queue create infos
    VkDeviceQueueCreateInfo* q_create_infos =
        (VkDeviceQueueCreateInfo*)malloc(unique_q_fams_count * sizeof(VkDeviceQueueCreateInfo));
    if(q_create_infos == NULL) {
        LOG_ERROR("Failed to allocate memory of size %lu: %s", unique_q_fams_count * sizeof(VkDeviceQueueCreateInfo),
                  strerror(errno));
        return false;
    }

    // Fill our device queue create info(s)
    float q_priority = 1.0f;
    for(uint32_t i = 0; i < unique_q_fams_count; ++i) {
        VkDeviceQueueCreateInfo q_create_info = {0};
        q_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q_create_info.queueFamilyIndex        = unique_q_fams[i]; // <-- Which queue family do we want
        q_create_info.queueCount              = 1; // <-- How many queues of this queue family do we want to create?
        q_create_info.pQueuePriorities        = &q_priority;
        q_create_infos[i]                     = q_create_info;
    }

    // Enable device features (non currently)
    VkPhysicalDeviceFeatures device_features = {0};
    device_features.samplerAnisotropy        = VK_FALSE;
    device_features.sampleRateShading        = VK_FALSE; // enable sample shading feature for the device

    // Start filling the main VkDeviceCreateInfo structure.
    VkDeviceCreateInfo create_dev_info   = {0};
    create_dev_info.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_dev_info.pQueueCreateInfos    = q_create_infos;
    create_dev_info.queueCreateInfoCount = unique_q_fams_count;

    create_dev_info.pEnabledFeatures = &device_features;

    // Enabeling device extensions, like swapchain
    create_dev_info.enabledExtensionCount   = device_extensions_count;
    create_dev_info.ppEnabledExtensionNames = device_extensions;

    if(vkCreateDevice(p_engine->physical_device, &create_dev_info, VK_NULL_HANDLE, &p_engine->device) != VK_SUCCESS) {
        LOG_ERROR("Failed to create vulkan logical device");
        return false;
    }
    LOG_INFO("Vulkan logical device created");

    deletion_queue_queue(p_engine->p_main_delq, (void*)p_engine, vkDestroyDevice_wrapper);

    // The queues are automatically created along with the logical device.
    // We can use the vkGetDeviceQueue function to retrieve queue handles for each queue family. The parameters are
    // the logical device, queue family, queue index and a pointer to the variable to store the queue handle in.
    // Because we’re only creating a single queue from "each" family, we’ll simply use index 0.
    vkGetDeviceQueue(p_engine->device, q_fam_indices.graphics_family, 0, &p_engine->graphics_queue);
    vkGetDeviceQueue(p_engine->device, q_fam_indices.present_family, 0, &p_engine->present_queue);

    p_engine->graphics_queue_index = q_fam_indices.graphics_family;
    p_engine->present_queue_index  = q_fam_indices.present_family;

    // Free dynamically allocated arrays
    free(unique_q_fams);
    unique_q_fams = NULL;
    free(q_create_infos);
    q_create_infos = NULL;
    return true;
}

static void vkDestroyDevice_wrapper(void* p_resource) {
    LOG_DEBUG("Callback: vkDestroyDevice_wrapper");
    vulkan_engine_t* p_engine = (vulkan_engine_t*)p_resource;
    vkDestroyDevice(p_engine->device, VK_NULL_HANDLE);
}

static bool create_swapchain(vulkan_engine_t* p_engine) {
    if(p_engine == NULL) {
        LOG_ERROR("create_swapchain: p_engine is NULL");
        return false;
    }

    // Swapchain support has already been checked but we run this function again to retrieve the swapchain support
    // details (the surface formats and present modes)
    swapchain_support_details_t swapchain_support = {0};
    if(!get_swapchain_support(p_engine, p_engine->physical_device, &swapchain_support)) {
        // TODO: Move LOG_ERROR to inside query_swapchain_support
        LOG_ERROR("Swapchain not supported by device");
        return false;
    }

    // Choose which surface formats we want to use
    VkSurfaceFormatKHR surface_format =
        choose_swapchain_surface_format(swapchain_support.formats, swapchain_support.formats_count);

    // Choose which present modes we want to use
    VkPresentModeKHR present_mode =
        choose_swapchain_present_mode(swapchain_support.present_modes, swapchain_support.present_modes_count);

    // Choose our swapchain extent
    VkExtent2D extent = choose_swapchain_extent(p_engine, swapchain_support.capabilities);

    // Check that the extent is non-zero
    if(extent.height == 0 && extent.width == 0) {
        LOG_ERROR("The swapchain extent is zero in one/both dimensions");
        return false;
    }

    // Aside from these properties we also have to decide how many images we would like to have in the swap chain.
    // The implementation specifies the minimum number that it requires to function. However, simply sticking to
    // this minimum means that we may sometimes have to wait on the driver to complete internal operations before we
    // can acquire another image to render to. Therefore it is recommended to request at least one more image than
    // the minimum.
    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;

    // We should also make sure to not exceed the maximum number of images while doing this, where 0 is a special
    // value that means that there is no maximum.
    if(swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount) {
        image_count = swapchain_support.capabilities.maxImageCount;
    }
    LOG_DEBUG("Minimum number of swapchain images to create: %u", image_count);

    // Create the swapchain
    VkSwapchainCreateInfoKHR create_swapchain_info = {0};
    create_swapchain_info.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    // Here we specify which surface the swapchain is tied to.
    create_swapchain_info.surface = p_engine->surface;
    // After specifying which surface the swapchain should be tied to, the details of the swapchain images are
    // specified.
    create_swapchain_info.minImageCount    = image_count;
    create_swapchain_info.imageFormat      = surface_format.format;
    create_swapchain_info.imageColorSpace  = surface_format.colorSpace;
    create_swapchain_info.imageExtent      = extent;
    create_swapchain_info.imageArrayLayers = 1;
    // create_swapchain_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // I am attempting to follow the cppvk13 tutorial where the swapchain is first created with this bit
    // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT is the default for vk-bootstrap
    create_swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    // The imageArrayLayers specifies the amount of layers each image consists of. This is always 1 unless you are
    // developing a stereoscopic 3D application. The imageUsage bit field specifies what kind of operations we’ll
    // use the images in the swap chain for. In this tutorial we’re going to render directly to them, which means
    // that they’re used as color attachment. It is also possible that you’ll render images to a separate image
    // first to perform operations like post-processing. In that case you may use a value like
    // VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use a memory operation to transfer the rendered image to a swap
    // chain image.
    queue_family_indices_t q_fam_indices = {0};
    get_queue_families(p_engine, p_engine->physical_device, &q_fam_indices);

    // If the queue families are not the same then they need to share the swapchain, if they are then theres no problem.
    uint32_t q_fam_indices_array[] = {q_fam_indices.graphics_family, q_fam_indices.present_family};
    if(q_fam_indices.graphics_family != q_fam_indices.present_family) {
        create_swapchain_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        create_swapchain_info.queueFamilyIndexCount = 2;
        create_swapchain_info.pQueueFamilyIndices   = q_fam_indices_array;
    } else {
        create_swapchain_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        create_swapchain_info.queueFamilyIndexCount = 0;              // Optional
        create_swapchain_info.pQueueFamilyIndices   = VK_NULL_HANDLE; // Optional
    }

    // We can specify that a certain transform should be applied to images in the swap chain if it is supported
    // (supportedTransforms in capabilities), like a 90 degree clockwise rotation or horizontal flip. To specify
    // that you do not want any transformation, simply specify the current transformation.
    create_swapchain_info.preTransform = swapchain_support.capabilities.currentTransform;

    // The compositeAlpha field specifies if the alpha channel should be used for blending with other windows in the
    // window system. You’ll almost always want to simply ignore the alpha channel, hence
    // VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR.
    create_swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_swapchain_info.presentMode    = present_mode;

    // If the clipped member is set to VK_TRUE then that means that we
    // don’t care about the color of pixels that are obscured, for example because another window is in front of
    // them. Unless you really need to be able to read these pixels back and get predictable results, you’ll get the
    // best performance by enabling clipping.
    create_swapchain_info.clipped = VK_TRUE;

    // That leaves one last field, oldSwapchain. With Vulkan it’s possible that your swap chain becomes invalid or
    // unoptimized while your application is running, for example because the window was resized. In that case the
    // swap chain actually needs to be recreated from scratch and a reference to the old one must be specified in
    // this field. This is a complex topic that we’ll learn more about in a future chapter. For now we’ll assume
    // that we’ll only ever create one swap chain.
    create_swapchain_info.oldSwapchain = VK_NULL_HANDLE;

    // Create swap chain.
    if(vkCreateSwapchainKHR(p_engine->device, &create_swapchain_info, VK_NULL_HANDLE, &p_engine->swapchain) !=
       VK_SUCCESS) {
        LOG_ERROR("Failed to create swapchain");
        return false;
    }
    LOG_INFO("Swapchain created");
    if(!deletion_queue_queue(p_engine->p_main_delq, p_engine, vkDestroySwapchainKHR_wrapper)) {
        LOG_ERROR("Failed to queue deletion node");
        vkDestroySwapchainKHR_wrapper(p_engine);
        return false;
    }

    // The swap chain has been created now, so all that remains is retrieving the handles of the VkImages in it.
    // We’ll reference these during rendering operations in later chapters. The images were created by the
    // implementation for the swapchain and they will be automatically cleaned up once the swap chain has been
    // destroyed, therefore we don’t need to add any cleanup code. Remember that we only specified a minimum number
    // of images in the swap chain, so the implementation is allowed to create a swapchain with more. That’s why
    // we’ll first query the final number of images with vkGetSwapchainImagesKHR, then resize the container and
    // finally call it again to retrieve the handles.
    vkGetSwapchainImagesKHR(p_engine->device, p_engine->swapchain, &image_count, VK_NULL_HANDLE);
    LOG_DEBUG("Number of swapchain images created: %u", image_count);

    // Allocate array to hole swapchain images
    p_engine->swapchain_images.p_images     = (VkImage*)malloc(image_count * sizeof(VkImage));
    p_engine->swapchain_images.images_count = image_count;
    if(!deletion_queue_queue(p_engine->p_main_delq, p_engine->swapchain_images.p_images, free_wrapper)) {
        LOG_ERROR("Failed to queue deletion node");
        free_wrapper(p_engine->swapchain_images.p_images);
        return false;
    }

    // Get swapchain images
    vkGetSwapchainImagesKHR(p_engine->device, p_engine->swapchain, &image_count, p_engine->swapchain_images.p_images);
    // Store the format and extent we’ve chosen for the swap chain images in member variables.
    p_engine->swapchain_image_format = surface_format.format;
    p_engine->swapchain_extent       = extent;

    // These are allocated in get swapchain support
    free(swapchain_support.formats);
    swapchain_support.formats = NULL;
    free(swapchain_support.present_modes);
    swapchain_support.present_modes = NULL;
    return true;
}

static void vkDestroySwapchainKHR_wrapper(void* p_engine) {
    LOG_DEBUG("Callback: vkDestroySwapchainKHR_wrapper");
    vkDestroySwapchainKHR(((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->swapchain,
                          VK_NULL_HANDLE);
}

static VkSurfaceFormatKHR choose_swapchain_surface_format(VkSurfaceFormatKHR* p_formats, size_t formats_count) {
    // For the color space we’ll use sRGB, which is pretty much the standard color space for viewing and printing
    // purposes, like the textures we’ll use later on. Because of that we should also use an sRGB color format, of
    // which one of the most common ones is VK_FORMAT_B8G8R8A8_SRGB.

    // Why not VK_FORMAT_B8G8R8A8_SRGB?
    // https://www.khronos.org/opengl/wiki/Image_Format
    // https://registry.khronos.org/vulkan/specs/latest/man/html/VkFormat.html

    for(size_t i = 0; i < formats_count; ++i) {
        if(p_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && // Used in the cppvk13 tutorial
           p_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return p_formats[i];
        }
    }
    // If that also fails then we could start ranking the available formats based on how "good" they are, but in
    // most cases it’s okay to just settle with the first format that is specified.
    return p_formats[0];
}

static VkPresentModeKHR choose_swapchain_present_mode(VkPresentModeKHR* p_present_modes, size_t present_modes_count) {
    // Only the VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to be available, so we’ll again have to write a
    // function that looks for the best mode that is available.
    // I personally think that VK_PRESENT_MODE_MAILBOX_KHR is a very nice trade-off if energy usage is not a
    // concern. It allows us to avoid tearing while still maintaining a fairly low latency by rendering new images
    // that are as up-to-date as possible right until the vertical blank.
    for(size_t i = 0; i < present_modes_count; ++i) {
        if(p_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return p_present_modes[i];
        }
    }
    // Garanteed so okay to return. This is most similar to vsync.
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D choose_swapchain_extent(vulkan_engine_t* p_engine, VkSurfaceCapabilitiesKHR capabilities) {
    // The swap extent is the resolution of the swap chain images and it’s almost always exactly equal to the
    // resolution of the window that we’re drawing to in pixels (more on that in a moment). The range of the
    // possible resolutions is defined in the VkSurfaceCapabilitiesKHR structure. Vulkan tells us to match the
    // resolution of the window by setting the width and height in the currentExtent member. However, some window
    // managers do allow us to differ here and this is indicated by setting the width and height in currentExtent to
    // a special value: the maximum value of uint32_t. In that case we’ll pick the resolution that best matches the
    // window within the minImageExtent and maxImageExtent bounds. But we must specify the resolution in the correct
    // unit.

    // GLFW uses two units when measuring sizes: pixels and screen coordinates. For example, the resolution {WIDTH,
    // HEIGHT} that we specified earlier when creating the window is measured in screen coordinates. But Vulkan
    // works with pixels, so the swap chain extent must be specified in pixels as well. Unfortunately, if you are
    // using a high DPI display (like Apple’s Retina display), screen coordinates don’t correspond to pixels.
    // Instead, due to the higher pixel density, the resolution of the window in pixel will be larger than the
    // resolution in screen coordinates. So if Vulkan doesn’t fix the swap extent for us, we can’t just use the
    // original {WIDTH, HEIGHT}. Instead, we must use glfwGetFramebufferSize to query the resolution of the window
    // in pixel before matching it against the minimum and maximum image extent.
    if(capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int width = 0, height = 0;
        // SDL_GetWindowSize(p_engine->p_SDL_window, &width, &height);
        if(!SDL_GetWindowSizeInPixels(p_engine->p_SDL_window, &width, &height)) {
            LOG_WARN("Failed to get the window size: %s", SDL_GetError());
            width  = 0;
            height = 0;
        }

        VkExtent2D actual_extent = {(uint32_t)width, (uint32_t)height};

        // We "clamp" the width and height so that they are within the bounds of the allowed min/max values supported by
        // the implementation
        if(actual_extent.width < capabilities.minImageExtent.width) {
            actual_extent.width = capabilities.minImageExtent.width;
        } else if(actual_extent.width > capabilities.maxImageExtent.width) {
            actual_extent.width = capabilities.maxImageExtent.width;
        }

        if(actual_extent.height < capabilities.minImageExtent.height) {
            actual_extent.height = capabilities.minImageExtent.height;
        } else if(actual_extent.height > capabilities.maxImageExtent.height) {
            actual_extent.height = capabilities.maxImageExtent.height;
        }

        return actual_extent;
    }
}

static bool create_image_views(vulkan_engine_t* p_engine) {
    if(p_engine == NULL) {
        LOG_ERROR("create_image_views: p_engine is NULL");
        return false;
    }

    // Allocate array to hold image views
    p_engine->swapchain_images.p_image_views =
        (VkImageView*)malloc(p_engine->swapchain_images.images_count * sizeof(VkImageView));
    if(p_engine->swapchain_images.p_image_views == NULL) {
        LOG_ERROR("Failed to allocate memory of size %lu: %s",
                  p_engine->swapchain_images.images_count * sizeof(VkImageView), strerror(errno));
    }
    if(!deletion_queue_queue(p_engine->p_main_delq, p_engine->swapchain_images.p_image_views, free_wrapper)) {
        LOG_ERROR("Failed to queue deletion node");
        free_wrapper(p_engine->swapchain_images.p_image_views);
        return false;
    }

    // create image views
    for(uint32_t i = 0; i < p_engine->swapchain_images.images_count; ++i) {
        // get_image_view(p_engine, p_engine->swapchain_images.p_image_views[i], p_engine->swapchain_images.p_images[i],
        //                  p_engine->swapchain_image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        VkImageViewCreateInfo view_info = {0};
        view_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image                 = p_engine->swapchain_images.p_images[i];

        // The viewType and format fields specify how the image data should be interpreted. The viewType parameter
        // allows you to treat images as 1D textures, 2D textures, 3D textures and cube maps.
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format   = p_engine->swapchain_image_format;

        // The components field allows you to swizzle the color channels around. For example, you can map all of the
        // channels to the red channel for a monochrome texture. You can also map constant values of 0 and 1 to a
        // channel. In our case we’ll stick to the default mapping.
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // The subresourceRange field describes what the image’s purpose is and which part of the image should be
        // accessed. Our images will be used as color targets without any mipmapping levels or multiple layers.
        view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel   = 0;
        view_info.subresourceRange.levelCount     = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount     = 1;

        // Create image view
        if(vkCreateImageView(p_engine->device, &view_info, VK_NULL_HANDLE,
                             &p_engine->swapchain_images.p_image_views[i]) != VK_SUCCESS) {
            LOG_ERROR("Failed to create image view");
            return false;
        }
    }

    if(!deletion_queue_queue(p_engine->p_main_delq, p_engine, vkDestroyImageView_array_wrapper)) {
        LOG_ERROR("Failed to queue deletion node");
        vkDestroyImageView_array_wrapper(p_engine);
        return false;
    }

    // CREATE DRAW IMAGE

    p_engine->draw_image.format  = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkExtent3D draw_image_extent = {p_engine->window_extent.width, p_engine->window_extent.height, 1};
    p_engine->draw_image.extent  = draw_image_extent;

    VkImageUsageFlags draw_image_usage = 0;

    draw_image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    draw_image_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    draw_image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    draw_image_usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    draw_image_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo img_info = {0};

    img_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.imageType     = VK_IMAGE_TYPE_2D;
    img_info.extent        = p_engine->draw_image.extent;
    img_info.extent.depth  = 1;
    img_info.mipLevels     = 1;
    img_info.arrayLayers   = 1;
    img_info.format        = p_engine->draw_image.format; // Or your needed format
    img_info.tiling        = VK_IMAGE_TILING_OPTIMAL;     // Usually optimal for GPU use
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;   // = 0 default value
    img_info.usage         = draw_image_usage;            // Example
    img_info.samples       = VK_SAMPLE_COUNT_1_BIT;
    img_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE; // = 0 default value

    if(vkCreateImage(p_engine->device, &img_info, VK_NULL_HANDLE, &p_engine->draw_image.image) != VK_SUCCESS) {
        LOG_ERROR("Failed to create draw image");
        return false;
    }

    // For the draw image, we want to allocate it on the GPU local memory
    // ALLOCATE MEMORY ON THE GPU

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(p_engine->device, p_engine->draw_image.image, &mem_req);

    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize       = mem_req.size;

    // Find a device local memory type
    uint32_t mem_type_index = 0;
    VkPhysicalDeviceMemoryProperties mem_prop;
    vkGetPhysicalDeviceMemoryProperties(p_engine->physical_device, &mem_prop);

    for(uint32_t i = 0; i < mem_prop.memoryTypeCount; ++i) {
        if((mem_req.memoryTypeBits & (1 << i)) &&
           (mem_prop.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ==
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            mem_type_index = i;
            break;
        }
    }

    alloc_info.memoryTypeIndex = mem_type_index;

    // VkDeviceMemory img_memory = NULL;
    vkAllocateMemory(p_engine->device, &alloc_info, VK_NULL_HANDLE, &p_engine->draw_image.mem);

    vkBindImageMemory(p_engine->device, p_engine->draw_image.image, p_engine->draw_image.mem, 0);

    // CREATE DRAW IMAGE VIEW

    VkImageViewCreateInfo img_view_info = {0};

    img_view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    img_view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    img_view_info.image                           = p_engine->draw_image.image;
    img_view_info.format                          = p_engine->draw_image.format;
    img_view_info.subresourceRange.baseMipLevel   = 0;
    img_view_info.subresourceRange.levelCount     = 1;
    img_view_info.subresourceRange.baseArrayLayer = 0;
    img_view_info.subresourceRange.layerCount     = 1;
    img_view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;

    if(vkCreateImageView(p_engine->device, &img_view_info, VK_NULL_HANDLE, &p_engine->draw_image.image_view) !=
       VK_SUCCESS) {
        LOG_ERROR("Failed to create draw image view");
        return false;
    }

    // ADD DESTRUCTION OF DRAW IMAGE AND DRAW IMAGE VIEW TO DELETION QUEUE

    if(!deletion_queue_queue(p_engine->p_main_delq, p_engine, vkDestroyImage_FreeMemory_wrapper)) {
        LOG_ERROR("Failed to queue deletion node");
        vkDestroyImage_FreeMemory_wrapper(p_engine);
        return false;
    }

    if(!deletion_queue_queue(p_engine->p_main_delq, p_engine, vkDestroyImageView_wrapper)) {
        LOG_ERROR("Failed to queue deletion node");
        vkDestroyImageView_wrapper(p_engine);
        return false;
    }

    LOG_INFO("Image views created");
    return true;
}

static bool get_image_view(vulkan_engine_t* p_engine, VkImageView image_view, VkImage image, VkFormat format,
                           VkImageAspectFlags aspect_flags, uint32_t mip_levels) {
    // TODO: Sanatize inputs

    // Fill image view create info
    VkImageViewCreateInfo view_info = {0};
    view_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image                 = image;

    // The viewType and format fields specify how the image data should be interpreted. The viewType parameter
    // allows you to treat images as 1D textures, 2D textures, 3D textures and cube maps.
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format   = format;

    // The components field allows you to swizzle the color channels around. For example, you can map all of the
    // channels to the red channel for a monochrome texture. You can also map constant values of 0 and 1 to a
    // channel. In our case we’ll stick to the default mapping.
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // The subresourceRange field describes what the image’s purpose is and which part of the image should be
    // accessed. Our images will be used as color targets without any mipmapping levels or multiple layers.
    view_info.subresourceRange.aspectMask     = aspect_flags;
    view_info.subresourceRange.baseMipLevel   = 0;
    view_info.subresourceRange.levelCount     = mip_levels;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount     = 1;

    // Create image view
    if(vkCreateImageView(p_engine->device, &view_info, VK_NULL_HANDLE, &image_view) != VK_SUCCESS) {
        LOG_ERROR("Failed to create image view");
        return false;
    }

    LOG_DEBUG("Image view created");
    return true;
}

static void vkDestroyImageView_array_wrapper(void* p_engine) {
    LOG_DEBUG("Callback: vkDestroyImageView_array_wrapper");
    for(uint32_t i = 0; i < ((vulkan_engine_t*)p_engine)->swapchain_images.images_count; ++i) {
        LOG_DEBUG("    Destroying image view, index: %u", i);
        vkDestroyImageView(((vulkan_engine_t*)p_engine)->device,
                           ((vulkan_engine_t*)p_engine)->swapchain_images.p_image_views[i], VK_NULL_HANDLE);
    }
}

static void vkDestroyImage_FreeMemory_wrapper(void* p_engine) {
    LOG_DEBUG("Callback: vkDestroyImage_FreeMemory_wrapper");
    vkDestroyImage(((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->draw_image.image,
                   VK_NULL_HANDLE); // Destroy the VkImage
    vkFreeMemory(((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->draw_image.mem,
                 VK_NULL_HANDLE); // Free the device memory
}

static void vkDestroyImageView_wrapper(void* p_engine) {
    LOG_DEBUG("Callback: vkDestroyImageView_wrapper");
    vkDestroyImageView(((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->draw_image.image_view,
                       VK_NULL_HANDLE);
}

static bool create_commands(vulkan_engine_t* p_engine) {
    if(p_engine == NULL) {
        LOG_ERROR("create_commands: p_engine is NULL");
        return false;
    }

    // Create a command pool for commands submitted to the graphics queue
    // We also want the pool to allow for resetting of individual command buffers

    VkCommandPoolCreateInfo cmd_pool_info = {0};

    cmd_pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_info.queueFamilyIndex = p_engine->graphics_queue_index;

    // Allocate the default command buffer that we will use for rendering
    VkCommandBufferAllocateInfo render_cmd_alloc_info = {0};

    render_cmd_alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    render_cmd_alloc_info.commandBufferCount = 1;
    render_cmd_alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    for(int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        if(vkCreateCommandPool(p_engine->device, &cmd_pool_info, VK_NULL_HANDLE, &p_engine->frames[i].cmd_pool) !=
           VK_SUCCESS) {
            LOG_ERROR("Failed to create frame command pool");
            return false;
        }
        render_cmd_alloc_info.commandPool = p_engine->frames[i].cmd_pool;
        if(vkAllocateCommandBuffers(p_engine->device, &render_cmd_alloc_info, &p_engine->frames[i].main_cmd_buffer) !=
           VK_SUCCESS) {
            LOG_ERROR("Failed to create frame command pool");
            return false;
        }
    }

    // Create immediate submit command pool and command buffer

    if(vkCreateCommandPool(p_engine->device, &cmd_pool_info, VK_NULL_HANDLE, &p_engine->imm_cmd_pool) != VK_SUCCESS) {
        LOG_ERROR("Failed to create immediate command pool");
        return false;
    }

    VkCommandBufferAllocateInfo imm_cmd_alloc_info = {0};

    imm_cmd_alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    imm_cmd_alloc_info.commandBufferCount = 1;
    imm_cmd_alloc_info.commandPool        = p_engine->imm_cmd_pool;
    imm_cmd_alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    if(vkAllocateCommandBuffers(p_engine->device, &imm_cmd_alloc_info, &p_engine->imm_cmd_buffer) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate immediate command buffer");
        return false;
    }

    if(!deletion_queue_queue(p_engine->p_main_delq, p_engine, vkDestroyCommandPool_wrapper)) {
        LOG_ERROR("Failed to queue deletion node");
        vkDestroyCommandPool_wrapper(p_engine);
        return false;
    }

    if(!deletion_queue_queue(p_engine->p_main_delq, p_engine, vkDestroyCommandPool_array_wrapper)) {
        LOG_ERROR("Failed to queue deletion node");
        vkDestroyCommandPool_array_wrapper(p_engine);
        return false;
    }

    LOG_INFO("Commands created");
    return true;
}

static void vkDestroyCommandPool_wrapper(void* p_engine) {
    LOG_DEBUG("Callback: vkDestroyCommandPool_wrapper");
    vkDestroyCommandPool(((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->imm_cmd_pool,
                         VK_NULL_HANDLE);
}

static void vkDestroyCommandPool_array_wrapper(void* p_engine) {
    LOG_DEBUG("Callback: vkDestroyCommandPool_array_wrapper");
    for(int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        LOG_DEBUG("    Destroying frame command pool, index: %d", i);
        vkDestroyCommandPool(((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->frames[i].cmd_pool,
                             VK_NULL_HANDLE);
    }
}

