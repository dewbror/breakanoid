#ifndef ERROR_H_
#define ERROR_H_

#include "config.h"

#define SUCCESS ((error_t){NULL, ERR_SRC_NONE, ERR_NONE})

/**
 * Enum for different error sources.
 */
typedef enum {
    ERR_SRC_NONE = 0, // Used for no error
    ERR_SRC_CORE,     // Error source is the core code
    ERR_SRC_VULKAN,   // Error source is from a call to a Vulkan function
    ERR_SRC_SDL       // Error source is from a call to an SDL function
} error_src_t;

/**
 * Core error codes.
 */
typedef enum {
    ERR_NONE = 0,

    // General errors
    ERR_TEMP, // CHANGE TO ERR_UNSUPPORTED
    ERR_NULL_ARG,
    ERR_MALLOC,
    ERR_UNSUPPORTED,

    // deletion stack
    ERR_DELETION_STACK_INIT,
    ERR_DELETION_STACK_PUSH,
    ERR_DELETION_STACK_FLUSH,

    // vulkan engine
    ERR_VULKAN_ENGINE_INIT,

    // vulkan instance
    ERR_VULKAN_DRIVER_VERSION,
    ERR_VULKAN_INSTANCE_DEBUG_MSG_INIT,

    // vulkan device
    ERR_VULKAN_SUPPORTED_DEVICE,
    ERR_SUITIBLE_DEVICE,
    ERR_VULKAN_PHYSICAL_DEVICE_INIT,
    ERR_VULKAN_DEVICE_INIT,

    // vulkan swapchain
    ERR_VULKAN_SWAPCHAIN_INIT,
    ERR_WINDOW_EXTENT,

    // vulkan image
    ERR_VULKAN_IMAGE
} core_error_code_t;

/**
 * An error struct that holds information about the error that has occured.
 */
typedef struct error_s {
    char* msg;
    error_src_t src;
    int code;
} error_t;

/**
 * \brief Initiate an error.
 *
 * \param[in] src The source of the error.
 * \param[in] code The error code.
 * \param[in] fmt A printf-style message format string.
 * \param[in] ... Additional parameters matching % tokens in the "fmt" string, if any.
 *
 * \warning This function allocates the msg field of error_t dynamically on the heap, use error_destroy() to free the
 * dynamically allocated memory. Do this directly after you have logged msg.
 */
error_t error_init(const error_src_t src, const int code, const char* fmt, ...) FORMAT_ATTR(3, 4);

/**
 * \brief Destroy an error.
 *
 * This function frees the dynamically allocated msg field of the error_t struct.
 */
void error_destroy(error_t* p_err);

#endif // ERROR_H_
