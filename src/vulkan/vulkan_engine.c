#include <alloca.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_vulkan.h>

#include "logger.h"

#include "error/error.h"
#include "error/sdl_error.h"

#include "vulkan/vulkan_types.h"
#include "vulkan/vulkan_instance.h"
#include "vulkan/vulkan_device.h"
#include "vulkan/vulkan_swapchain.h"
#include "vulkan/vulkan_image.h"
#include "vulkan/vulkan_cmd.h"
#include "vulkan/vulkan_sync.h"
#include "vulkan_descriptor.h"
#include "vulkan/vulkan_engine.h"

#include "util/deletion_stack.h"

#include "SDL/sdl_backend.h"

#define HEIGHT 1080
#define WIDTH 1920

static const uint32_t device_extensions_count = 1;
static const char* const device_extensions[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static const uint32_t instance_layers_count = 0;

#ifdef NDEBUG
static const bool enable_validation_layers = false;
#else
static const bool enable_validation_layers = true;
#endif

/**
 * A struct for deleting the vulkan render surface.
 * */
typedef struct surface_del_struct_s {
    VkInstance instance;
    VkSurfaceKHR surface;
} surface_del_struct_t;

/**
 * \brief Destroy vulkan render surface
 *
 * \param[in] p_void_surface_del_struct Pointer to a sufrace_del_struct_t containing the surface to be destroyed.
 */
static void surface_destroy(void* p_void_surface_del_struct);

error_t vulkan_engine_init(vulkan_engine_t* p_engine) {
    // Check if p_engine is NULL
    if(p_engine == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: p_engine is NULL", __func__);

    // Set window extent, this will be performed from some settings file in the future.
    p_engine->window_extent.height = HEIGHT;
    p_engine->window_extent.width = WIDTH;

    // Allocate main deletion queue
    p_engine->p_main_del_stack = deletion_stack_init();
    if(p_engine->p_main_del_stack == NULL)
        return error_init(ERR_SRC_CORE, ERR_DELETION_STACK_INIT, "%s: Failed to initiate deletion stack", __func__);

    error_t err;

    // Initialize SDL
    err =
        sdl_backend_init(&p_engine->p_window, (int)p_engine->window_extent.width, (int)p_engine->window_extent.height);
    if(err.code != 0)
        return err;

    err = deletion_stack_push(p_engine->p_main_del_stack, p_engine->p_window, sdl_backend_destroy);
    if(err.code != 0) {
        sdl_backend_destroy(p_engine->p_window);
        return err;
    }

    // Create vulkan instance
    err = vulkan_instance_init(&p_engine->instance);
    if(err.code != 0)
        return err;

    // Possibly move deletion to inside the init functions
    err = deletion_stack_push(p_engine->p_main_del_stack, p_engine->instance, vulkan_instance_destroy);
    if(err.code != 0) {
        vulkan_instance_destroy(p_engine->instance);
        return err;
    }

    // Setup debug messenger
#ifndef NDEBUG
    err = vulkan_instance_debug_msg_init(p_engine->instance, &p_engine->debug_msg);
    if(err.code != 0)
        return err;

    vulkan_instance_debug_msg_del_struct_t* p_debug_msg_del_struct =
        (vulkan_instance_debug_msg_del_struct_t*)malloc(sizeof(vulkan_instance_debug_msg_del_struct_t));
    p_debug_msg_del_struct->instance = p_engine->instance;
    p_debug_msg_del_struct->debug_msg = p_engine->debug_msg;

    err = deletion_stack_push(p_engine->p_main_del_stack, p_debug_msg_del_struct, vulkan_instance_debug_msg_destroy);
    if(err.code != 0) {
        vulkan_instance_debug_msg_destroy(p_debug_msg_del_struct);
        return err;
    }
#endif

    // Create SDL window surface
    if(!SDL_Vulkan_CreateSurface(p_engine->p_window, p_engine->instance, VK_NULL_HANDLE, &p_engine->surface))
        return error_init(ERR_SRC_SDL, SDL_ERR_VULKAN_CREATE_SURFACE, "Failed to create vulkan rendering surface: %s",
            SDL_GetError());
    LOG_INFO("Vulkan rendering surface created");

    surface_del_struct_t* p_surface_del_struct = (surface_del_struct_t*)malloc(sizeof(surface_del_struct_t));
    p_surface_del_struct->instance = p_engine->instance;
    p_surface_del_struct->surface = p_engine->surface;

    err = deletion_stack_push(p_engine->p_main_del_stack, p_surface_del_struct, surface_destroy);
    if(err.code != 0)
        return err;

    // Maybe move into vulkan_device_init?
    err = vulkan_physical_device_init(p_engine->instance, p_engine->surface, &p_engine->physical_device);
    if(err.code != 0)
        return err;

    err = vulkan_device_init(p_engine->surface, p_engine->physical_device, &p_engine->device, &p_engine->queues);
    if(err.code != 0)
        return err;

    err = deletion_stack_push(p_engine->p_main_del_stack, p_engine->device, vulkan_device_destroy);
    if(err.code != 0) {
        vulkan_device_destroy(p_engine->device);
        return err;
    }

    err = vulkan_swapchain_init(p_engine->device, p_engine->physical_device, p_engine->surface, p_engine->p_window,
        &p_engine->vulkan_swapchain);
    if(err.code != 0)
        return err;

    vulkan_swapchain_del_struct_t* p_swapchain_del_struct =
        (vulkan_swapchain_del_struct_t*)malloc(sizeof(vulkan_swapchain_del_struct_t));
    p_swapchain_del_struct->device = p_engine->device;
    p_swapchain_del_struct->vulkan_swapchain = p_engine->vulkan_swapchain;

    err = deletion_stack_push(p_engine->p_main_del_stack, p_swapchain_del_struct, vulkan_swapchain_destroy);
    if(err.code != 0) {
        vulkan_swapchain_destroy(p_swapchain_del_struct);
        return err;
    }

    err = vulkan_image_create(p_engine->device, p_engine->physical_device, p_engine->window_extent.width,
        p_engine->window_extent.height, &p_engine->draw_image);
    if(err.code != 0) {
        LOG_ERROR("Failed to create image");
        return err;
    }

    allocated_image_del_strut_t* p_allocated_image_del_struct =
        (allocated_image_del_strut_t*)malloc(sizeof(allocated_image_del_strut_t));
    p_allocated_image_del_struct->device = p_engine->device;
    p_allocated_image_del_struct->allocated_image = p_engine->draw_image;

    err = deletion_stack_push(p_engine->p_main_del_stack, p_allocated_image_del_struct, vulkan_image_destroy);
    if(err.code != 0) {
        vulkan_image_destroy(p_allocated_image_del_struct);
        return err;
    }

    // Create commands
    err = vulkan_cmd_frame_init(p_engine->device, &p_engine->queues, p_engine->p_frames);
    if(err.code != 0)
        return err;

    err = vulkan_cmd_imm_init(p_engine->device, &p_engine->queues, &p_engine->imm_cmd_pool, &p_engine->imm_cmd_buffer);
    if(err.code != 0)
        return err;

    for(int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        cmd_pool_del_struct_t* p_cmd_pool = (cmd_pool_del_struct_t*)malloc(sizeof(cmd_pool_del_struct_t));
        p_cmd_pool->device = p_engine->device;
        p_cmd_pool->cmd_pool = p_engine->p_frames[i].cmd_pool;

        err = deletion_stack_push(p_engine->p_main_del_stack, p_cmd_pool, vulkan_cmd_pool_destroy);
        if(err.code != 0) {
            vulkan_cmd_pool_destroy(p_cmd_pool);
            return err;
        }
    }

    cmd_pool_del_struct_t* p_cmd_pool = (cmd_pool_del_struct_t*)malloc(sizeof(cmd_pool_del_struct_t));
    p_cmd_pool->device = p_engine->device;
    p_cmd_pool->cmd_pool = p_engine->imm_cmd_pool;

    err = deletion_stack_push(p_engine->p_main_del_stack, p_cmd_pool, vulkan_cmd_pool_destroy);
    if(err.code != 0) {
        vulkan_cmd_pool_destroy(p_cmd_pool);
        return err;
    }

    // Create sync structures
    err = vulkan_sync_frame_init(p_engine->device, p_engine->p_frames);
    if(err.code != 0)
        return err;

    vulkan_sync_frame_del_struct_t* p_frame = malloc(sizeof(vulkan_sync_frame_del_struct_t));
    p_frame->device = p_engine->device;
    p_frame->p_frames = p_engine->p_frames;

    err = deletion_stack_push(p_engine->p_main_del_stack, p_frame, vulkan_sync_frame_destroy);
    if(err.code != 0) {
        vulkan_sync_frame_destroy(p_frame);
        return err;
    }

    // Create sync structures
    err = vulkan_sync_imm_init(p_engine->device, &p_engine->imm_fence);
    if(err.code != 0)
        return err;

    fence_del_struct_t* p_fence = (fence_del_struct_t*)malloc(sizeof(fence_del_struct_t));
    p_fence->device = p_engine->device;
    p_fence->fence = p_engine->imm_fence;

    err = deletion_stack_push(p_engine->p_main_del_stack, p_fence, vulkan_sync_fence_destroy);
    if(err.code != 0) {
        vulkan_sync_fence_destroy(p_fence);
        return err;
    }

    err = vulkan_descriptor_init(p_engine->device, &p_engine->draw_image, &p_engine->desc_alloc,
        &p_engine->draw_img_desc, &p_engine->draw_img_desc_layout);
    if(err.code != 0)
        return err;

    vulkan_desc_del_t* p_vulkan_desc_del = (vulkan_desc_del_t*)malloc(sizeof(vulkan_desc_del_t));
    p_vulkan_desc_del->device = p_engine->device;
    p_vulkan_desc_del->pool = p_engine->desc_alloc.pool;
    p_vulkan_desc_del->desc_layout = p_engine->draw_img_desc_layout;

    err = deletion_stack_push(p_engine->p_main_del_stack, p_vulkan_desc_del, vulkan_descriptor_destroy);
    if(err.code != 0) {
        vulkan_descriptor_destroy(p_vulkan_desc_del);
        return err;
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
    return SUCCESS;
}

error_t vulkan_engine_destroy(vulkan_engine_t* p_engine) {
    // Check if p_engine is NULL
    if(p_engine == NULL) {
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: p_engine is NULL", __func__);
    }

    if(p_engine->device != NULL) {
        // Make sure the GPU has become idle before flushing deletion queue
        vkDeviceWaitIdle(p_engine->device);
    }

    // Flush deletion queue
    error_t err = deletion_stack_flush(&p_engine->p_main_del_stack);
    if(err.code != 0) {
        return err;
    }

    // Vulkan engine successfully destroyed
    LOG_INFO("Vulkan engine destroyed");
    return SUCCESS;
}

static void surface_destroy(void* p_void_surface_del_struct) {
    LOG_DEBUG("Callback: %s", __func__);

    if(p_void_surface_del_struct == NULL) {
        LOG_ERROR("%s: sufrace_del_struct is NULL", __func__);
        return;
    }

    // Cast pointer
    surface_del_struct_t* p_surface_del_struct = (surface_del_struct_t*)p_void_surface_del_struct;

    if(p_surface_del_struct->instance == NULL) {
        LOG_ERROR("%s: instance is NULL", __func__);
        return;
    }

    if(p_surface_del_struct->surface == NULL) {
        LOG_ERROR("%s: surface is NULL", __func__);
        return;
    }

    // Destroy surface
    vkDestroySurfaceKHR(p_surface_del_struct->instance, p_surface_del_struct->surface, VK_NULL_HANDLE);

    free(p_surface_del_struct);
    p_surface_del_struct = NULL;
    p_void_surface_del_struct = NULL;
}
