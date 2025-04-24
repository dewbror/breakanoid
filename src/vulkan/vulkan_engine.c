/*
  vulkan_engine.c
*/

#include "version.h"
#include "types.h"
#include "vulkan_engine.h"
#include "util/deletion_queue.h"
#include "SDL/SDL_backend.h"

// Validation layers need to be enabled by specifying their name. All of the useful standard validation are bundled into a layer included in the SDK that is known as VK_LAYER_KHRONOS_validation.
static const uint32_t validation_layers_count = 1;
static const char *validation_layers[1] = {"VK_LAYER_KHRONOS_validation"};

// Swapchain support is not part of the Vulkan core. We need to enable the VK_KHR_swapchain device extension.
static const uint32_t device_extensions_count = 1;
static const char *device_extensions[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
static const bool enable_validation_layers = false;
#else
static const bool enable_validation_layers = true;
#endif

/**
 * Struct for holding the graphics queue family index and the present queue family index.
 */
typedef struct queue_family_indices {
    uint32_t graphics_family;
    uint32_t present_family;
} queue_family_indices;

/**
 * Struct used for finding the swapchain support details.
 */
typedef struct swapchain_support_details {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    size_t formats_count;
    VkPresentModeKHR *present_modes;
    size_t present_modes_count;
} swapchain_support_details;

/**
 * Create a vulkan instance.
 * 
 * \param[in] p_engine Pointer to the vulkan_engine.
 * 
 * \return True if successful, false if failed.
 */
static bool create_instance(vulkan_engine *p_engine);

/**
 * Function for checking validation layer support. Checks if all layers in the validation_layers array are available, this is done by comparing validation_layers to the array of layers given by vkEnumerateInstanceLayerProperties. This function is meant to only be called if enable_validation_layers = true, which is only the case in the debug build.
 * 
 * \return True if successful, false if failed.
 */
static bool check_validation_layer_support(void);

/**
 * Function for aquiring the required instance extensions needed of rvkCreateInstance, this is queried using SDL_Vulkan_GetInstanceExtensions. If enable_validation_layer = true, will add VK_EXT_DEBUG_UTILS_EXTENSION_NAME to the returned array of required instance extensions. The returned array of required instance extensions is allocated using SDL_malloc and must be freed using SDL_free.
 * 
 * \param[in, out] p_required_extensions_count The number of required extensions.
 * 
 * \return A pointer to a dynamically allocated array of strings (const char *).
 */
static const char **get_required_extensions(uint32_t *p_required_extensions_count);

/**
 * Function for populating the VkDebugUtilsMessengerCreateInfoEXT struct.
 * 
 * \param[in, out] p_create_info Pointer to the VkDebugUtilsMessengerCreateInfoEXT to populate.
 */
static void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT *p_create_info);

/**
 * A callback function to be used by the vulkan validation layers.
 * 
 * More information about this function is available in comments inside the function definition.
 * 
 * \param[in] messageSeverity Specifies the severity of the message.
 * \param[in] messageType     Specifies the type of the message.
 * \param[in] p_callback_data Pointer to a VkDebugUtilsMessengerCallbackDataEXT struct containing the details of the message itself.
 * \param[in] p_user_data     Pointer specified during the setup of the callback and allows you to pass your own data to it.
 * 
 * \return VK_FALSE.
 */
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT /*message_severity*/, VkDebugUtilsMessageTypeFlagsEXT /*message_type*/, const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data, void * /*p_user_data*/);

/**
 * vkDestroyInstance wrapper to be used in the deletion_queue.
 * 
 * \param[in] p_vulkan_instance Pointer to the vulkan_instance.
 */
static void vkDestroyInstance_wrapper(void *p_vulkan_instance);

/**
 * If enable_validation_layers = true, will populate a VkDebugUtilsMessengerCreateInfoEXT using populate_debug_messenger_create_info and create a VkDebugUtilsMessengerEXT object using vkCreateDebugUtilsMessengerEXT. The VkDebugUtilsMessengerCreateInfoEXT must be destroyed using DestroyDebugUtilsMessengerEXT.
 * 
 * \param[in] p_engine Pointer to the vulkan_engine.
 * 
 * \return True if successful, false if failed.
 */
static bool setup_debug_messenger(vulkan_engine *p_engine);

/**
 * Proxy function for vkCreateDebugUtilsMessengerEXT.
 * 
 * \return VkResult.
 */
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *p_create_info, const VkAllocationCallbacks *p_allocator, VkDebugUtilsMessengerEXT *p_debug_messenger);

/**
 * Proxy function for vkDestroyDebugUtilsMessengerEXT.
 */
static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks *p_allocator);

/**
 * DestroyDebugUtilsMessengerEXT wrapper for use in deletion queue.
 * 
 * \param[in] p_vulkan_engine Pointer to vulkan engine.
 */
static void DestroyDebugUtilsMessengerEXT_wrapper(void *p_engine);

/**
 * Function for picking a physical device.
 * 
 * \param[in] p_engine Pointer to vulkan_engine.
 * 
 * \return True if successful, false if failed.
 */
static bool pick_physical_device(vulkan_engine *p_engine);

static bool is_device_suitable(vulkan_engine *p_engine, VkPhysicalDevice device);
static bool find_queue_families(vulkan_engine *p_engine, VkPhysicalDevice device, queue_family_indices *q_fam_indices);
static bool check_device_extension_support(VkPhysicalDevice device);
static bool query_swapchain_support(vulkan_engine *p_engine, VkPhysicalDevice device, swapchain_support_details *swapchain_support);
static void free_wrapper(void *mem);

static bool create_logical_device(vulkan_engine *p_engine);

static bool create_swapchain(vulkan_engine *p_engine);
static VkSurfaceFormatKHR choose_swapchain_surface_format(VkSurfaceFormatKHR *formats, size_t formats_count);
static VkPresentModeKHR choose_swapchain_present_mode(VkPresentModeKHR *present_modes, size_t present_modes_count);
static VkExtent2D choose_swapchain_extent(vulkan_engine *p_engine, VkSurfaceCapabilitiesKHR capabilities);

static bool create_image_views(vulkan_engine *p_engine);
static VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels);

bool vulkan_engine_init(vulkan_engine *p_engine) {
    
    // Set window extent, this will be performed from some settings file in the future.
    p_engine->window_extent.height = 1080;
    p_engine->window_extent.width = 1920;

    // Allocate main deletion queue
    p_engine->p_main_delq = deletion_queue_alloc();

    // Initialize SDL
    if(!init_SDL_backend(p_engine)) {
        printf("Failed to init SDL\n");
        return false;
    }

    // Create vulkan instance
    if(!create_instance(p_engine)) {
        printf("Failed to create vulkan instance\n");
        return false;
    }

    // Setup debug messenger
    if(!setup_debug_messenger(p_engine)) {
        printf("Failed to setup debug messenger\n");
        return false;
    }

    // Create SDL window surface
    if(!SDL_Vulkan_CreateSurface(p_engine->p_SDL_window, p_engine->instance, VK_NULL_HANDLE, &p_engine->surface)) {
        printf("failed to create SDL window surface. Error: %s\n", SDL_GetError());
        return false;
    }

    // Pick a physical device (GPU)
    if(!pick_physical_device(p_engine)) {
        printf("Failed to find suitible physical device\n");
        return false;
    }

    // Create logical device and get queue families
    if(!create_logical_device(p_engine)) {
        printf("Failed to create logical device\n");
        return false;
    }

    // Create swapchain
    if(!create_swapchain(p_engine)) {
        printf("Failed to create swapchain\n");
        return false;
    }

    // Create image views
    // if(!create_image_views(p_engine)) {
    //     printf("Failed to create image views\n");
    //     return false;
    // }

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

    return true;
}

void vulkan_engine_destroy(vulkan_engine *p_engine) {
    // Flush deletion queue
    deletion_queue_flush(p_engine->p_main_delq);
}

static bool create_instance(vulkan_engine *p_engine) {
    // Checks if all of the requested layers are available.
    if(enable_validation_layers && !check_validation_layer_support()) {
        printf("Validation layers requested, but not available\n");
        return false;
    }

    // Retrieve a list of supported extensions before creating an instance.
    // Request just the number of extensions.
    uint32_t extension_count;
    vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &extension_count, VK_NULL_HANDLE);
    VkExtensionProperties *extensions = (VkExtensionProperties*)malloc(extension_count*sizeof(VkExtensionProperties));

    // Query the extension details.
    vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &extension_count, extensions);

    printf("\nAvailable extensions:\n");
    for(uint32_t i = 0; i < extension_count; ++i) {
        printf("%s\n", extensions[i].extensionName);
    }
    printf("\n");
    // Free extensions array
    free(extensions);

    VkApplicationInfo app_info  = {0};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName   = "Breakanoid";
    app_info.applicationVersion = VK_MAKE_VERSION(breakanoid_VERSION_MAJOR, breakanoid_VERSION_MINOR, breakanoid_VERSION_PATCH);
    app_info.pEngineName        = "No Engine";
    app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_inst_info = {0};
    create_inst_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_inst_info.pApplicationInfo = &app_info;

    uint32_t count_required_extensions = 0;
    const char **req_extensions        = get_required_extensions(&count_required_extensions);
    // createInfo.enabledExtensionCount        = static_cast<uint32_t>(reqExtensions.size());
    create_inst_info.enabledExtensionCount   = count_required_extensions;
    create_inst_info.ppEnabledExtensionNames = req_extensions;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};

    // Include validation layer names if they are enabled.
    if(enable_validation_layers) {
        create_inst_info.enabledLayerCount   = validation_layers_count;
        create_inst_info.ppEnabledLayerNames = validation_layers;

        populate_debug_messenger_create_info(&debug_create_info);
        create_inst_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info;
    } else {
        create_inst_info.enabledLayerCount = 0;
        create_inst_info.pNext             = VK_NULL_HANDLE;
    }

    // Create instance.
    printf("Create vulkan instance:\n");
    if(vkCreateInstance(&create_inst_info, VK_NULL_HANDLE, &p_engine->instance) != VK_SUCCESS) {
        return false;
    }
    printf("\n");
    deletion_queue_queue(p_engine->p_main_delq, &p_engine->instance, vkDestroyInstance_wrapper);

    // Need to handle this better, this is dynamically allocated in getRequiredExtensions and must be freed after
    // instance creation.
    SDL_free(req_extensions);

    return true;
}

static bool check_validation_layer_support(void) {
    uint32_t available_layers_count;
    // Query number of available layers.
    vkEnumerateInstanceLayerProperties(&available_layers_count, VK_NULL_HANDLE);

    // Query available layers
    VkLayerProperties *available_layers = (VkLayerProperties*)malloc(available_layers_count*sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&available_layers_count, available_layers);
    
    printf("\nAvailable layers:\n");
    for(uint32_t i = 0; i < available_layers_count; ++i)
        printf("%s\n", available_layers[i].layerName);
    
    // Check if all of the layers in validationLayers exist in the availableLayers list.
    for(uint32_t i = 0; i < validation_layers_count; ++i) {
        bool layer_found = false;
        for(uint32_t j = 0; j < available_layers_count; ++j) {
            if(strcmp(validation_layers[i], available_layers[j].layerName) == 0) {
                layer_found = true;
                break;
            }
        }
        if(!layer_found) {
            return false;
        }
    }

    free(available_layers);
    return true;
}

static const char **get_required_extensions(uint32_t *p_required_extensions_count) {
    // Specify the desired global extensions.
    uint32_t count_instance_extensions = 0;
    uint32_t count_extensions          = 0;
    const char *const *instance_extensions = SDL_Vulkan_GetInstanceExtensions(&count_instance_extensions);

    count_extensions = count_instance_extensions;
    if(enable_validation_layers)
        count_extensions += 1;

    // This malloc needs to be freed manually outside this function!!!
    const char **extensions = (const char **)SDL_malloc(count_extensions * sizeof(const char *));
    SDL_memcpy(&extensions[0], instance_extensions, count_instance_extensions * sizeof(const char *));

    if(enable_validation_layers) {
        extensions[count_extensions - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }
    // Set the extenion count
    *p_required_extensions_count = count_extensions;
    return extensions;
}

static void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT *p_create_info) {
    p_create_info->sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    p_create_info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    p_create_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    p_create_info->pfnUserCallback = debug_callback;
    p_create_info->pUserData       = VK_NULL_HANDLE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data, void *p_user_data) {
    // UNUSED
    (void)message_severity;
    (void)message_type;
    (void)p_user_data;
    
    // The first parameter specifies the severity of the message, which is one of the following flags:
    // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Diagnostic message.
    // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: Informational message like the creation of a resource.
    // VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: Message about behavior that is not necessarily an error, but
    // very likely a bug in your application.
    // VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: Message about behavior
    // that is invalid and may cause crashes.
    // The values of this enumeration are set up in such a way that you can use a comparison operation to check if a
    // message is equal or worse compared to some level of severity.

    // The messageType parameter can have the following values:
    // VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: Some event has happened that is unrelated to the specification
    // or performance.
    // VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: Something has happened that violates the
    // specification or indicates a possible mistake.
    // VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: Potential non-optimal use of Vulkan.

    // The pCallbackData parameter refers to a VkDebugUtilsMessengerCallbackDataEXT struct containing the details of
    // the message itself, with the most important members being:
    //  - pMessage: The debug message as a null-terminated string.
    //  - pObjects: Array of Vulkan object handles related to the message.
    //  - objectCount: Number of objects in array.

    // Finally, the pUserData parameter contains a pointer that was specified during the setup of the callback and
    // allows you to pass your own data to it.

    printf("validation layer: %s\n", p_callback_data->pMessage);
    return VK_FALSE;
}

static void vkDestroyInstance_wrapper(void *p_vulkan_instance) {
    printf("Callback: vkDestroyInstance_wrapper\n");
    vkDestroyInstance(*((VkInstance*)p_vulkan_instance), VK_NULL_HANDLE);
}

static bool setup_debug_messenger(vulkan_engine *p_engine) {
    // No debug messenger if validation layers are not enabled
    if(!enable_validation_layers)
        return true;

    // Fill struct with details about the messenger and its callback.
    VkDebugUtilsMessengerCreateInfoEXT create_info = {0};
    populate_debug_messenger_create_info(&create_info);

    // This struct should be passed to the vkCreateDebugUtilsMessengerEXT function to create the
    // VkDebugUtilsMessengerEXT object. Unfortunately, because this function is an extension function, it is not
    // automatically loaded. We have to look up its address ourselves using vkGetInstanceProcAddr. We’re going to
    // create our own proxy function that handles this in the background.
    // Call proxy function:
    if(CreateDebugUtilsMessengerEXT(p_engine->instance, &create_info, VK_NULL_HANDLE, &p_engine->debug_messenger) != VK_SUCCESS) {
        return false;
    }
    // The VkDebugUtilsMessengerEXT object also needs to be cleaned up with a call to
    // vkDestroyDebugUtilsMessengerEXT, which we also need to aquire via a proxy function.
    // We call the detroyer in cleanup.
    deletion_queue_queue(p_engine->p_main_delq, p_engine, DestroyDebugUtilsMessengerEXT_wrapper);
    return true;
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *p_create_info,
    const VkAllocationCallbacks *p_allocator, VkDebugUtilsMessengerEXT *p_debug_messenger) {
    // Look up address of vkCreateDebugUtilsMessengerEXT
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // return it if not NULL, else return error extension not present.
    if(func != VK_NULL_HANDLE) {
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
    const VkAllocationCallbacks *p_allocator) {
    // Make sure that this function is either a static class function or a function outside the class.

    // Look up address of vkDestroyDebugUtilsMessengerEXT
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    // return it if not NULL
    if(func != VK_NULL_HANDLE)
        func(instance, debug_messenger, p_allocator);
}

static void DestroyDebugUtilsMessengerEXT_wrapper(void *p_vulkan_engine) {
    printf("Callback: DestroyDebugUtilsMessengerEXT_wrapper\n");
    DestroyDebugUtilsMessengerEXT(((vulkan_engine*)p_vulkan_engine)->instance, ((vulkan_engine*)p_vulkan_engine)->debug_messenger, VK_NULL_HANDLE);
}

static bool pick_physical_device(vulkan_engine *p_engine) {
    uint32_t device_count;
    // Get number of devices with Vulkan support
    vkEnumeratePhysicalDevices(p_engine->instance, &device_count, VK_NULL_HANDLE);
    // If no deives with Vulkan support, runtime error
    if(device_count == 0) {
        printf("Failed to find GPUs with Vulkan support\n");
        return false;
    }
    // Allocate array holding all supported devices
    VkPhysicalDevice *devices = (VkPhysicalDevice*)malloc(device_count*sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(p_engine->instance, &device_count, devices);

    // Check for suitable device in devices
    printf("Looking for suitable devices:\n");
    for(uint32_t i = 0; i < device_count; ++i) {
        if(is_device_suitable(p_engine, devices[i])) {
            p_engine->physical_device = devices[i];
            // msaa_samples    = get_maxUsable_sample_count(p_engine->physical_device);
            p_engine->msaa_samples = VK_SAMPLE_COUNT_1_BIT;
            break;
        }
    }

    // Free devices array
    free(devices);

    // If no suitable device was found, runtime error
    if(p_engine->physical_device == VK_NULL_HANDLE) {
        printf("Failed to find suitable GPU\n");
        return false;
    }
    printf("MSAA samples: %d\n", p_engine->msaa_samples);
    return true;
}

static bool is_device_suitable(vulkan_engine *p_engine, VkPhysicalDevice device) {
    // Query basic device properties like name and supported Vulkan version
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);
    // The support for optional features like texture compression, 64 bit floats and multi viewport rendering
    // (useful for VR) can be queried using vkGetPhysicalDeviceFeatures:
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(device, &device_features);

    printf("Device name: %s\n", device_properties.deviceName);
    printf("Device supported Vulkan version: %u.%u.%u.%u\n", VK_API_VERSION_VARIANT(device_properties.apiVersion), VK_API_VERSION_MAJOR(device_properties.apiVersion), VK_API_VERSION_MINOR(device_properties.apiVersion), VK_API_VERSION_PATCH(device_properties.apiVersion));

    // Check if graphics and present queue families are supported by device
    queue_family_indices q_fam_indices = {0};
    if(!find_queue_families(p_engine, device, &q_fam_indices)) {
        printf("Required queue families not supported by device\n");
        return false;
    }

    // Check if required extensions are supported by device
    if(!check_device_extension_support(device)) {
        printf("Required device extensions not supported\n");
        return false;
    }

    swapchain_support_details swapchain_support = {0};
    bool swapchain_adequate = query_swapchain_support(p_engine, device, &swapchain_support);
    // Swap chain support is sufficient for this tutorial if there is at least one supported image format and
    // one supported presentation mode given the window surface we have.
    // It is important that we only try to query for swap chain support after verifying that the extension is
    // available!!!

    // These are allocated in query swapchain support and must be freed!!! not ideal solution.
    free(swapchain_support.formats);
    free(swapchain_support.present_modes);

    VkPhysicalDeviceFeatures supported_features;
    vkGetPhysicalDeviceFeatures(device, &supported_features);
    return  swapchain_adequate && supported_features.samplerAnisotropy;
}

static bool find_queue_families(vulkan_engine *p_engine, VkPhysicalDevice device, queue_family_indices *q_fam_indices) {
    // It has been briefly touched upon before that almost every operation in Vulkan, anything from drawing to
    // uploading textures, requires commands to be submitted to a queue. There are different types of queues that
    // originate from different queue families and each family of queues allows only a subset of commands. For
    // example, there could be a queue family that only allows processing of compute commands or one that only
    // allows memory transfer related commands. We need to check which queue families are supported by the device
    // and which one of these supports the commands that we want to use. For that purpose we’ll add a new function
    // findQueueFamilies that looks for all the queue families we need.
    // queue_family_indices q_fam_indices;

    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, VK_NULL_HANDLE);
    VkQueueFamilyProperties *queue_families = (VkQueueFamilyProperties*)malloc(queue_family_count*sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    bool got_graphics, got_present = false;
    for(uint32_t i = 0; i < queue_family_count; ++i) {
        VkBool32 present_support = false;
        
        // Check if the device has support for presentation queue
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, p_engine->surface, &present_support);
        
        if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            q_fam_indices->graphics_family = i;
            // printf("graphics queue family index: %u\n", i);
            got_graphics = true;
        }
        if(present_support) {
            q_fam_indices->present_family = i;
            // printf("present queue family index:  %u\n", i);
            got_present = true;
        }
        if(got_graphics && got_present)
            return true;
    }
    // Note that it’s very likely that these end up being the same queue family after all, but throughout the
    // program we will treat them as if they were separate queues for a uniform approach. Nevertheless, you could
    // add logic to explicitly prefer a physical device that supports drawing and presentation in the same queue for
    // improved performance.
    return false;
}

static bool check_device_extension_support(VkPhysicalDevice device) {
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &extension_count, VK_NULL_HANDLE);

    VkExtensionProperties *available_extensions = (VkExtensionProperties*)malloc(extension_count*sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(device, VK_NULL_HANDLE, &extension_count, available_extensions);

    for(uint32_t i = 0; i < device_extensions_count; ++i) {
        bool req_ext_found = false;
        for(uint32_t j = 0; j < extension_count; ++j) {
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

static bool query_swapchain_support(vulkan_engine *p_engine, VkPhysicalDevice device, swapchain_support_details *p_details) {
    // Just checking if a swap chain is available is not sufficient, because it may not actually be compatible with
    // our window surface. Creating a swap chain also involves a lot more settings than instance and device
    // creation, so we need to query for some more details before we’re able to proceed.
    // swapchain_support_details details;

    // Query basic surface capabilities.
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, p_engine->surface, &p_details->capabilities);

    // Query supported surface formats.
    uint32_t formats_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, p_engine->surface, &formats_count, VK_NULL_HANDLE);
    if(formats_count != 0) {
        p_details->formats = (VkSurfaceFormatKHR*)malloc(formats_count*sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, p_engine->surface, &formats_count, p_details->formats);
        p_details->formats_count = formats_count;
        // deletion_queue_queue(p_engine->p_main_delq, details->formats, free_wrapper);
    } else {
        return false;
    }

    // Query supported presentation modes.
    uint32_t present_modes_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, p_engine->surface, &present_modes_count, VK_NULL_HANDLE);
    if(present_modes_count != 0) {
        p_details->present_modes = (VkPresentModeKHR*)malloc(present_modes_count*sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, p_engine->surface, &present_modes_count, p_details->present_modes);
        p_details->present_modes_count = present_modes_count;
        // deletion_queue_queue(p_engine->p_main_delq, details->present_modes, free_wrapper);
    } else {
        return false;
    }

    return true;
}

static void free_wrapper(void *p_mem) {
    printf("Callback: free_wrapper\n");
    free(p_mem);
}

static bool create_logical_device(vulkan_engine *p_engine) {
    queue_family_indices q_fam_indices = {0};
    if(!find_queue_families(p_engine, p_engine->physical_device, &q_fam_indices)) {
        printf("required queue families not found\n");
        return false;
    }
    
    // Hard coded, make dynamic in future?
    uint32_t unique_q_fams_count = 0;
    if(q_fam_indices.graphics_family == q_fam_indices.present_family) {
        unique_q_fams_count = 1;
    } else {
        unique_q_fams_count = 2;
    }
    uint32_t *unique_q_fams = (uint32_t*)malloc(unique_q_fams_count*sizeof(uint32_t));
    if(unique_q_fams_count == 1) {
        unique_q_fams[0] = q_fam_indices.graphics_family;
    } else if(unique_q_fams_count == 2) {
        unique_q_fams[0] = q_fam_indices.graphics_family;
        unique_q_fams[1] = q_fam_indices.present_family;
    } else {
        return false;
    }

    VkDeviceQueueCreateInfo *q_create_infos = (VkDeviceQueueCreateInfo*)malloc(unique_q_fams_count*sizeof(VkDeviceQueueCreateInfo));
    float q_priority = 1.0f;
    for(uint32_t i = 0; i < unique_q_fams_count; ++i) {
        VkDeviceQueueCreateInfo q_create_info = {0};
        q_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q_create_info.queueFamilyIndex = unique_q_fams[i]; // <-- Which queue family do we want
        q_create_info.queueCount       = 1;                // <-- How many queues of this queue family do we want to create?
        q_create_info.pQueuePriorities = &q_priority;
        q_create_infos[i] = q_create_info;
    }

    VkPhysicalDeviceFeatures device_features = {0};
    device_features.samplerAnisotropy = VK_FALSE;
    device_features.sampleRateShading = VK_FALSE; // enable sample shading feature for the device
    // Now we can start filling the main VkDeviceCreateInfo strucutre.
    VkDeviceCreateInfo create_dev_info = {0};
    create_dev_info.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_dev_info.pQueueCreateInfos    = q_create_infos;
    create_dev_info.queueCreateInfoCount = unique_q_fams_count;

    create_dev_info.pEnabledFeatures = &device_features;

    // Enabeling device extensions, like swapchain
    create_dev_info.enabledExtensionCount   = device_extensions_count;
    create_dev_info.ppEnabledExtensionNames = device_extensions;

    if(vkCreateDevice(p_engine->physical_device, &create_dev_info, VK_NULL_HANDLE, &p_engine->device) != VK_SUCCESS) {
        printf("Failed to create logical device\n");
    }

    // The queues are automatically created along with the logical device.
    // We can use the vkGetDeviceQueue function to retrieve queue handles for each queue family. The parameters are
    // the logical device, queue family, queue index and a pointer to the variable to store the queue handle in.
    // Because we’re only creating a single queue from this family, we’ll simply use index 0.
    vkGetDeviceQueue(p_engine->device, q_fam_indices.graphics_family, 0, &p_engine->graphics_queue);
    vkGetDeviceQueue(p_engine->device, q_fam_indices.present_family, 0, &p_engine->present_queue);

    free(unique_q_fams);
    free(q_create_infos);
    return true;
}

static bool create_swapchain(vulkan_engine *p_engine) {
    swapchain_support_details swapchain_support = {0};
    if(!query_swapchain_support(p_engine, p_engine->physical_device, &swapchain_support)) {
        printf("Physical device does not support swapchain\n");
        return false;
    }

    VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(swapchain_support.formats, swapchain_support.formats_count);
    VkPresentModeKHR present_mode     = choose_swapchain_present_mode(swapchain_support.present_modes, swapchain_support.present_modes_count);
    VkExtent2D extent                 = choose_swapchain_extent(p_engine, swapchain_support.capabilities);

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

    // Lets create the swap chain object, the Vulkan way, with a create info struct ofc!
    VkSwapchainCreateInfoKHR create_swapchain_info = {0};
    create_swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    // Here we specify which surface the swap chain is tied to.
    create_swapchain_info.surface = p_engine->surface;
    // After specifying which surface the swap chain should be tied to, the details of the swap chain images are
    // specified.
    create_swapchain_info.minImageCount    = image_count;
    create_swapchain_info.imageFormat      = surface_format.format;
    create_swapchain_info.imageColorSpace  = surface_format.colorSpace;
    create_swapchain_info.imageExtent      = extent;
    create_swapchain_info.imageArrayLayers = 1;
    // create_swapchain_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_swapchain_info.imageUsage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    // The imageArrayLayers specifies the amount of layers each image consists of. This is always 1 unless you are
    // developing a stereoscopic 3D application. The imageUsage bit field specifies what kind of operations we’ll
    // use the images in the swap chain for. In this tutorial we’re going to render directly to them, which means
    // that they’re used as color attachment. It is also possible that you’ll render images to a separate image
    // first to perform operations like post-processing. In that case you may use a value like
    // VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use a memory operation to transfer the rendered image to a swap
    // chain image.
    queue_family_indices q_fam_indices = {0};
    find_queue_families(p_engine, p_engine->physical_device, &q_fam_indices);

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
    if(vkCreateSwapchainKHR(p_engine->device, &create_swapchain_info, VK_NULL_HANDLE, &p_engine->swapchain) != VK_SUCCESS) {
        printf("Failed to create swap chain\n");
    }
    // The swap chain has been created now, so all that remains is retrieving the handles of the VkImages in it.
    // We’ll reference these during rendering operations in later chapters. The images were created by the
    // implementation for the swap chain and they will be automatically cleaned up once the swap chain has been
    // destroyed, therefore we don’t need to add any cleanup code. Remember that we only specified a minimum number
    // of images in the swap chain, so the implementation is allowed to create a swap chain with more. That’s why
    // we’ll first query the final number of images with vkGetSwapchainImagesKHR, then resize the container and
    // finally call it again to retrieve the handles.
    vkGetSwapchainImagesKHR(p_engine->device, p_engine->swapchain, &image_count, VK_NULL_HANDLE);

    p_engine->swapchain_images.p_images = (VkImage*)malloc(image_count*sizeof(VkImage));
    p_engine->swapchain_images.sz = image_count;
    deletion_queue_queue(p_engine->p_main_delq, p_engine->swapchain_images.p_images, free_wrapper);

    vkGetSwapchainImagesKHR(p_engine->device, p_engine->swapchain, &image_count, p_engine->swapchain_images.p_images);
    // Store the format and extent we’ve chosen for the swap chain images in member variables.
    p_engine->swapchain_image_format = surface_format.format;
    p_engine->swapchain_extent       = extent;

    // These are allocated in query swapchain support and must be freed!!! not ideal solution.
    free(swapchain_support.formats);
    free(swapchain_support.present_modes);
    return true;
}

static VkSurfaceFormatKHR choose_swapchain_surface_format(VkSurfaceFormatKHR *p_formats, size_t formats_count) {
    // For the color space we’ll use sRGB, which is pretty much the standard color space for viewing and printing
    // purposes, like the textures we’ll use later on. Because of that we should also use an sRGB color format, of
    // which one of the most common ones is VK_FORMAT_B8G8R8A8_SRGB.

    // Why not VK_FORMAT_B8G8R8A8_SRGB?
    // https://www.khronos.org/opengl/wiki/Image_Format
    // https://registry.khronos.org/vulkan/specs/latest/man/html/VkFormat.html

    for(size_t i = 0; i < formats_count; ++i) {
        if(p_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
            p_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return p_formats[i];
        }
    }
    // If that also fails then we could start ranking the available formats based on how "good" they are, but in
    // most cases it’s okay to just settle with the first format that is specified.
    return p_formats[0];
}

static VkPresentModeKHR choose_swapchain_present_mode(VkPresentModeKHR *p_present_modes, size_t present_modes_count) {
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

static VkExtent2D choose_swapchain_extent(vulkan_engine *p_engine, VkSurfaceCapabilitiesKHR capabilities) {
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
        int width, height;
        SDL_GetWindowSize(p_engine->p_SDL_window, &width, &height);

        VkExtent2D actual_extent = {(uint32_t)width, (uint32_t)height};

        // The clamp function is used here to bound the values of width and height between the allowed minimum and
        // maximum extents that are supported by the implementation.
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

static bool create_image_views(vulkan_engine *p_engine) {
    p_engine->swapchain_images.p_image_views = (VkImageView*)malloc(p_engine->swapchain_images.sz*sizeof(VkImageView));
    deletion_queue_queue(p_engine->p_main_delq, p_engine->swapchain_images.p_image_views, free_wrapper);

    for(uint32_t i = 0; i < p_engine->swapchain_images.sz; ++i) {
        p_engine->swapchain_images.p_image_views[i] = create_image_view(p_engine->swapchain_images.p_images[i], p_engine->swapchain_image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

static VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels) {
    VkImageViewCreateInfo view_info = {0};
    return NULL;
}
