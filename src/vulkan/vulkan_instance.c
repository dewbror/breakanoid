#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_vulkan.h>

#include "version.h"
#include "logger.h"
#include "vulkan/vulkan_instance.h"

static const uint32_t instance_layers_count = 0;

#ifdef NDEBUG
static const bool enable_validation_layers = false;
#else
static const bool enable_validation_layers = true;
#endif

/**
 * Aquire the required instance layers. If enable_validation_layer = true, will add "VK_LAYER_KHRONOS_validation" to
 * the returned array. The returned array of required instance extensions is allocated
 * using malloc and must be freed using.
 *
 * \param[out] p_required_layers_count The number of required layers.
 *
 * \return Pointer to a dynamically allocated array of strings listing the required layers.
 *
 * \note The currently only required layer is the validation layers if they are enabled, else there are none.
 */
static const char** get_required_layers(uint32_t* p_required_layers_count);

/**
 * Aquire the required instance extensions needed in vkCreateInstance, this is queried using
 * SDL_Vulkan_GetInstanceExtensions. If enable_validation_layer = true, will add VK_EXT_DEBUG_UTILS_EXTENSION_NAME to
 * the returned array of required instance extensions. The returned array of required instance extensions is allocated
 * using malloc and must be freed using.
 *
 * \param[out] p_required_extensions_count The number of required extensions.
 *
 * \return Pointer to a dynamically allocated array of strings listing the required extensions.
 */
static const char** get_required_extensions(uint32_t* p_required_extensions_count);

/**
 * Get the VkDebugUtilsMessengerCreateInfoEXT struct.
 *
 * \param[out] p_create_info Pointer to the VkDebugUtilsMessengerCreateInfoEXT to populate.
 */
static void get_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT* p_create_info);

/**
 * A callback function to be used by the vulkan validation layers.
 *
 * More information about this function is available in comments inside the function definition.
 *
 * \param[in] messageSeverity Specifies the severity of the message.
 * \param[in] messageType Specifies the type of the message.
 * \param[in] p_callback_data Pointer to a VkDebugUtilsMessengerCallbackDataEXT struct containing the details of the
 * message itself. \param[in] p_user_data Pointer specified during the setup of the callback and allows you to pass
 * your own data to it.
 *
 * \return VK_FALSE.
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data);

/**
 * Proxy function for vkCreateDebugUtilsMessengerEXT.
 *
 * \return VkResult.
 */
static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
    const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger);

/**
 * Proxy function for vkDestroyDebugUtilsMessengerEXT.
 */
static void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* p_allocator);

bool vulkan_instance_init(VkInstance* p_instance) {
    // Check if p_engine is NULL
    if(p_instance == NULL) {
        LOG_ERROR("vulkan_instance_init: instance is NULL");
        return false;
    }

    // If validation layers are enabled, check if they are suuported
    // if(enable_validation_layers && !check_validation_layer_support()) {
    //     LOG_ERROR("Validation layers requested but not available. Requesting validation layers when not avaiable is "
    //               "currently not supported\n");
    //     return false;
    // }

    // Get the available vulkan driver version
    uint32_t api_version = 0;
    vkEnumerateInstanceVersion(&api_version);
    LOG_DEBUG(
        "Available Vulkan API version: %uv%u.%u.%u", VK_API_VERSION_VARIANT(api_version),
        VK_API_VERSION_MAJOR(api_version), VK_API_VERSION_MINOR(api_version), VK_API_VERSION_PATCH(api_version));

    // Must be greater than 1.3
    if(api_version < VK_MAKE_VERSION(1, 3, 0)) {
        LOG_ERROR("Vulkan driver version 1.3+ required");
        return false;
    }

    // Create application info
    VkApplicationInfo app_info = {0};

    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Breakanoid";
    app_info.applicationVersion = VK_MAKE_VERSION(break_VERSION_MAJOR, break_VERSION_MINOR, break_VERSION_PATCH);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = api_version;

    // Create instance create info
    VkInstanceCreateInfo create_inst_info = {0};

    create_inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_inst_info.pApplicationInfo = &app_info;

    uint32_t required_layers_count = 0;
    uint32_t required_extensions_count = 0;

    // Query required instance layers
    const char** required_layers = get_required_layers(&required_layers_count);
    create_inst_info.enabledLayerCount = required_layers_count;
    create_inst_info.ppEnabledLayerNames = (const char* const*)required_layers;

    // Query required instance extensions (From SDL_Vulkan_GetInstanceExtensions)
    const char** required_extensions = get_required_extensions(&required_extensions_count);
    create_inst_info.enabledExtensionCount = required_extensions_count;
    create_inst_info.ppEnabledExtensionNames = (const char* const*)required_extensions;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
    VkLayerSettingsCreateInfoEXT layer_settings_create_info = {0};

    // Enable non-default validation layer settings
    const VkBool32 setting_validate_sync = VK_TRUE;
    const VkBool32 setting_validate_best_practices = VK_TRUE;
    const VkBool32 setting_enable_message_limit = VK_TRUE;
    const int32_t setting_duplicate_message_limit = 3;

    if(enable_validation_layers) {
        const VkLayerSettingEXT settings[] = {
            {"VK_LAYER_KHRONOS_validation",           "validate_sync", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1,
             &setting_validate_sync          },
            {"VK_LAYER_KHRONOS_validation", "validate_best_practices", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1,
             &setting_validate_best_practices},
            {"VK_LAYER_KHRONOS_validation",    "enable_message_limit", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1,
             &setting_enable_message_limit   },
            {"VK_LAYER_KHRONOS_validation", "duplicate_message_limit",  VK_LAYER_SETTING_TYPE_INT32_EXT, 1,
             &setting_duplicate_message_limit}
        };

        layer_settings_create_info.sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT;
        layer_settings_create_info.settingCount = 2;
        layer_settings_create_info.pSettings = settings;

        // get debug messenger create info
        get_debug_messenger_create_info(&debug_create_info);

        // The current order of the pNext chain of extenion structs is
        // The order of the pNext chain should not matter. However, using the reverse order:
        //      instance -> layer_settings -> debug_utils

        // create_inst_info.pNext = &debug_create_info;
        create_inst_info.pNext = &layer_settings_create_info;

        // debug_create_info.pNext = &layer_settings_create_info;
        layer_settings_create_info.pNext = &debug_create_info;
    }

    uint32_t req_inst_layers_count = 0;

    // Create instance.
    if(vkCreateInstance(&create_inst_info, VK_NULL_HANDLE, p_instance) != VK_SUCCESS) {
        // Handle vkCreateInstance error
        LOG_ERROR("Failed to create vulkan instance");
        return false;
    }
    LOG_INFO("Vulkan instance created");

    // Free dynamically allocated arrays
    free(required_layers); // NOLINT(bugprone-multi-level-implicit-pointer-conversion)
    required_layers = NULL;
    free(required_extensions); // NOLINT(bugprone-multi-level-implicit-pointer-conversion)
    required_extensions = NULL;

    return true;
}

void vulkan_instance_destroy(void* p_void_instance) {
    LOG_DEBUG("Callback: vulkan_instance_destroy");

    // NULL check
    if(p_void_instance == NULL) {
        LOG_ERROR("vulkan_instance_destroy: instance is NULL");
        return;
    }

    // Cast pointer
    VkInstance instance = (VkInstance)p_void_instance;

    // Destroy instance
    vkDestroyInstance(instance, VK_NULL_HANDLE);
}

static const char** get_required_layers(uint32_t* p_required_instance_layers_count) {
    // Since the only instance layers we are currently requiring are the validation layers (if they are enabled) the
    // first thing we do is check if validaiton layers are enabled, if not then we return NULL. The design of this
    // function is supposed to mimic that of get_required_extensions.

    *p_required_instance_layers_count = 0;

    // Query number of available instance layers.
    uint32_t available_layers_count = 0;
    if(vkEnumerateInstanceLayerProperties(&available_layers_count, VK_NULL_HANDLE) != VK_SUCCESS) {
        // Handle vkEnumerateInstanceLayerProperties error
        LOG_ERROR("Failed to query number of available instance layers");
        return NULL;
    }

    // Allocate array of VkLayerProperties with size available_layers_count
    VkLayerProperties* available_layers =
        (VkLayerProperties*)malloc(available_layers_count * sizeof(VkLayerProperties));
    if(available_layers == NULL) {
        LOG_ERROR(
            "Failed to allocate memory of size %lu: %s", available_layers_count * sizeof(VkLayerProperties),
            strerror(errno));
        return NULL;
    }

    // Query available instance layers
    vkEnumerateInstanceLayerProperties(&available_layers_count, available_layers);

#ifndef NDEBUG
    // List avaiable instance layers
    LOG_DEBUG("Available instance layers:");
    for(uint32_t i = 0; i < available_layers_count; ++i) {
        LOG_DEBUG("    %s", available_layers[i].layerName);
    }
#endif

    uint32_t allocated_layers_count = instance_layers_count;
    if(enable_validation_layers) {
        ++allocated_layers_count;
    } else {
        return NULL;
    }

    // Allocate memory for the extensions array
    const char** layers = (const char**)malloc(allocated_layers_count * sizeof(char*));
    if(layers == NULL) {
        // Handle memory allocation failure
        LOG_ERROR("Failed to allocate array of size %lu: %s", allocated_layers_count * sizeof(char*), strerror(errno));
        return NULL;
    }

    // Add validation layers if they are enabled
    if(enable_validation_layers) {
        layers[allocated_layers_count - 1] = "VK_LAYER_KHRONOS_validation";
    }

#ifndef NDEBUG
    // List required instance extensions
    LOG_DEBUG("Required instance layers:");
    for(uint32_t i = 0; i < allocated_layers_count; ++i) {
        LOG_DEBUG("    %s", layers[i]);
    }
#endif

    // Check if all of the layers in validation_layers exist in the available_layers list.
    for(uint32_t i = 0; i < allocated_layers_count; ++i) {
        bool layer_found = false;
        for(uint32_t j = 0; j < available_layers_count; ++j) {
            if(strcmp(layers[i], available_layers[j].layerName) == 0) {
                layer_found = true;
                break;
            }
        }
        if(!layer_found) {
            LOG_ERROR("Instance layer: %s, is not available", layers[i]);
            return NULL;
        }
    }

    // Free allocated VkLayerProperties array
    free(available_layers);
    available_layers = NULL;

    // Set the layers count
    *p_required_instance_layers_count = allocated_layers_count;

    // NOTE: THIS ARRAY IS ALLOCATED ON THE HEAP AND MUST BE FREED
    return layers;
}

static const char** get_required_extensions(uint32_t* p_required_extensions_count) {
    *p_required_extensions_count = 0;

    // Query number of available instance extensions
    uint32_t available_extensions_count = 0;
    vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &available_extensions_count, VK_NULL_HANDLE);

    // Allocate array of VkExtensionProperties with size available_extension_count
    VkExtensionProperties* available_extensions =
        (VkExtensionProperties*)malloc(available_extensions_count * sizeof(VkExtensionProperties));
    if(available_extensions == NULL) {
        LOG_ERROR(
            "Failed to allocate memory of size %lu: %s", available_extensions_count * sizeof(VkExtensionProperties),
            strerror(errno));
        return NULL;
    }

    // Query available instance extensions
    vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &available_extensions_count, available_extensions);

#ifndef NDEBUG
    // List available instance extension names
    LOG_DEBUG("Available instance extensions:");
    for(uint32_t i = 0; i < available_extensions_count; ++i) {
        LOG_DEBUG("    %s", available_extensions[i].extensionName);
    }
#endif

    uint32_t SDL_extensions_count = 0;

    // https://wiki.libsdl.org/SDL3/SDL_Vulkan_GetInstanceExtensions
    // Query the required instance extensions from SDL
    const char* const* SDL_extensions = SDL_Vulkan_GetInstanceExtensions(&SDL_extensions_count);
    if(SDL_extensions == NULL) {
        // Handle failure to query required instance extensions from SDl
        LOG_ERROR("Failed to query the required instance extensions from SDL: %s", SDL_GetError());
        return NULL;
    }

    // If validation layers are enabled, increase the extensions count by 1
    uint32_t allocated_extensions_count = SDL_extensions_count;
    if(enable_validation_layers) {
        ++allocated_extensions_count;
    }

    // Allocate memory for the extensions array
    const char** extensions = (const char**)malloc(allocated_extensions_count * sizeof(char*));
    if(extensions == NULL) {
        // Handle memory allocation failure
        LOG_ERROR(
            "Failed to allocate array of size %lu: %s", allocated_extensions_count * sizeof(char*), strerror(errno));
        return NULL;
    }

    // Copy instance extensions into the allocated array
    // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
    memcpy(extensions, SDL_extensions, SDL_extensions_count * sizeof(char*));

    // Add debug utils extension if validation layers are enabled
    if(enable_validation_layers) {
        extensions[allocated_extensions_count - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }

#ifndef NDEBUG
    // List required instance extensions
    LOG_DEBUG("Required instance extensions:");
    for(uint32_t i = 0; i < allocated_extensions_count; ++i) {
        LOG_DEBUG("    %s", extensions[i]);
    }
#endif

    // Check if all of the extensions in the available_extensions array exist.
    for(uint32_t i = 0; i < allocated_extensions_count; ++i) {
        bool extension_found = false;
        for(uint32_t j = 0; j < available_extensions_count; ++j) {
            if(strcmp(extensions[i], available_extensions[j].extensionName) == 0) {
                extension_found = true;
                break;
            }
        }
        if(!extension_found) {
            LOG_ERROR("Instance extension: %s, is not available", extensions[i]);
            return NULL;
        }
    }

    // Free allocated extensions array
    free(available_extensions);
    available_extensions = NULL;

    // Set the extension count
    *p_required_extensions_count = allocated_extensions_count;

    // Return the extensions array as immutable (const char* const*)
    // NOTE: THIS ARRAY IS ALLOCATED ON THE HEAP AND MUST BE FREED
    return extensions;
}

static void get_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT* p_create_info) {
    p_create_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    p_create_info->messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    p_create_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    p_create_info->pfnUserCallback = debug_callback;
    p_create_info->pUserData = VK_NULL_HANDLE;
    p_create_info->pNext = VK_NULL_HANDLE;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data) {
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

    int level = LOG_LEVEL_TRACE;
    if(message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        level = LOG_LEVEL_ERROR;
    } else if(message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        level = LOG_LEVEL_WARN;
        // } else if(message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        //     level = LOG_LEVEL_INFO;
    } else if(message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        level = LOG_LEVEL_DEBUG;
    } else {
        return VK_FALSE;
    }

    logger__msg(level, NULL, 0, "validation layer: %s", p_callback_data->pMessage);
    return VK_FALSE;
}

bool vulkan_instance_debug_msg_init(VkInstance instance, VkDebugUtilsMessengerEXT* p_debug_msg) {
    if(instance == NULL) {
        LOG_ERROR("vulkan_instance_debug_messenger_init: instance is NULL");
        return false;
    }

    if(p_debug_msg == NULL) {
        LOG_ERROR("vulkan_instance_debug_messenger_init: p_debug_msg is NULL");
        return false;
    }

    // No debug messenger if validation layers are not enabled
    if(!enable_validation_layers) {
        *p_debug_msg = NULL;
        return true;
    }

    // Fill struct with details about the messenger and its callback.
    VkDebugUtilsMessengerCreateInfoEXT create_info = {0};
    get_debug_messenger_create_info(&create_info);

    // This struct should be passed to the vkCreateDebugUtilsMessengerEXT function to create the
    // VkDebugUtilsMessengerEXT object. Unfortunately, because this function is an extension function, it is not
    // automatically loaded. We have to look up its address ourselves using vkGetInstanceProcAddr. We’re going to
    // create our own proxy function that handles this in the background.
    // Call proxy function:
    if(CreateDebugUtilsMessengerEXT(instance, &create_info, VK_NULL_HANDLE, p_debug_msg) != VK_SUCCESS) {
        // Handle CreateDebugUtilsMessenger error
        LOG_WARN("Failed to create debug messenger");

        // Set the debug messenger to NULL if failed to be created
        *p_debug_msg = NULL;
        return true;
    }

    LOG_INFO("Debug messenger initiated");
    return true;
}

void vulkan_instance_debug_msg_destroy(void* p_void_debug_msg_del_struct) {
    LOG_DEBUG("Callback: vulkan_instance_debug_messenger_destroy");

    // The VkDebugUtilsMessengerEXT object also needs to be cleaned up with a call to
    // vkDestroyDebugUtilsMessengerEXT, which we also need to aquire via a proxy function.
    // We call the detroyer in cleanup.

    // NULL check
    if(p_void_debug_msg_del_struct == NULL) {
        LOG_ERROR("vulkan_instance_debug_msg_destroy: debug_msg_del_struct is NULL");
        return;
    }

    // Cast struct pointer
    vulkan_instance_debug_msg_del_struct_t* p_debug_msg_del_struct =
        (vulkan_instance_debug_msg_del_struct_t*)p_void_debug_msg_del_struct;

    if(p_debug_msg_del_struct->instance == NULL) {
        LOG_ERROR("vulkan_instance_destroy: instance is NULL");
        return;
    }

    if(p_debug_msg_del_struct->debug_msg == NULL) {
        LOG_ERROR("vulkan_instance_destroy: debug_msg is NULL")
    }

    // Destroy debug messenger
    DestroyDebugUtilsMessengerEXT(p_debug_msg_del_struct->instance, p_debug_msg_del_struct->debug_msg, VK_NULL_HANDLE);

    // Free deletion struct pointer
    free(p_void_debug_msg_del_struct);
}

static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
    const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger) {
    // Look up address of vkCreateDebugUtilsMessengerEXT
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // return it if not NULL, else return error extension not present.
    if(func != VK_NULL_HANDLE) {
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* p_allocator) {
    // Make sure that this function is either a static class function or a function outside the class.

    // Look up address of vkDestroyDebugUtilsMessengerEXT
    PFN_vkDestroyDebugUtilsMessengerEXT func =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    // return it if not NULL
    if(func != VK_NULL_HANDLE) {
        func(instance, debug_messenger, p_allocator);
    }
}
