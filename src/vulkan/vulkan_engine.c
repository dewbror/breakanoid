#include <alloca.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_vulkan.h>

#include "logger.h"
#include "vulkan/vulkan_types.h"
#include "vulkan/vulkan_instance.h"
#include "vulkan/vulkan_device.h"
#include "vulkan/vulkan_swapchain.h"
#include "vulkan/vulkan_image.h"
#include "vulkan/vulkan_cmd.h"
#include "vulkan/vulkan_engine.h"
#include "util/deletion_stack.h"
#include "SDL/SDL_backend.h"

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

/**
 * Create sync structures
 *
 * \param[in] p_engine Pointer to vulkan engine.
 */
static bool create_sync_structs(vulkan_engine_t* p_engine);

/**
 * vkDestroyFence wrapper.
 *
 * \param[in] p_engine Pointer to the vulkan engine
 */
static void vkDestroyFence_wrapper(void* p_engine);

/**
 * vkDestroyFence and vkDestroySemaphore wrapper for frame array sync structs.
 *
 * \param[in] p_engine Pointer to the vulkan engine
 */
static void vkDestroyFence_Sem_wrapper(void* p_engine);

/**
 * Create descriptors.
 *
 * \param[in] p_engine Pointer to the vulkan engine.
 */
static bool create_descriptors(vulkan_engine_t* p_engine);

bool vulkan_engine_init(vulkan_engine_t* p_engine) {
    // Check if p_engine is NULL
    if(p_engine == NULL) {
        LOG_ERROR("vulkan_engine_init: p_engine is NULL");
        return false;
    }

    // Set window extent, this will be performed from some settings file in the future.
    p_engine->window_extent.height = HEIGHT;
    p_engine->window_extent.width = WIDTH;

    // Allocate main deletion queue
    p_engine->p_main_del_stack = deletion_stack_init();
    // This NULL check is technically unnecessary since it is also performed inside deletion_queue_alloc
    if(p_engine->p_main_del_stack == NULL) {
        LOG_ERROR("Vulkan engine deletion queue is NULL after allocation");
        return false;
    }

    // Initialize SDL
    if(!SDL_backend_init(
           &p_engine->p_SDL_window, (int)p_engine->window_extent.width, (int)p_engine->window_extent.height)) {
        LOG_ERROR("Failed to initialize SDL backend");
        return false;
    }

    if(!deletion_stack_push(p_engine->p_main_del_stack, p_engine->p_SDL_window, SDL_backend_destroy)) {
        LOG_ERROR("Failed to push deletion node");
        SDL_backend_destroy(p_engine->p_SDL_window);
        return false;
    }

    // Create vulkan instance
    if(!vulkan_instance_init(&p_engine->instance)) {
        LOG_ERROR("Failed to create vulkan instance");
        return false;
    }

    // Possibly move deletion to inside the init functions
    if(!deletion_stack_push(p_engine->p_main_del_stack, p_engine->instance, vulkan_instance_destroy)) {
        LOG_ERROR("Failed to push deletion node");
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
    p_debug_msg_del_struct->instance = p_engine->instance;
    p_debug_msg_del_struct->debug_msg = p_engine->debug_msg;

    if(!deletion_stack_push(p_engine->p_main_del_stack, p_debug_msg_del_struct, vulkan_instance_debug_msg_destroy)) {
        LOG_ERROR("Failed to push deletion callback");
        vulkan_instance_debug_msg_destroy(p_debug_msg_del_struct);
        return false;
    }

    // Create SDL window surface
    if(!SDL_Vulkan_CreateSurface(p_engine->p_SDL_window, p_engine->instance, VK_NULL_HANDLE, &p_engine->surface)) {
        LOG_ERROR("Failed to create vulkan rendering surface: %s", SDL_GetError());
        return false;
    }
    LOG_INFO("Vulkan rendering surface created");

    surface_del_struct_t* p_surface_del_struct = (surface_del_struct_t*)malloc(sizeof(surface_del_struct_t));
    p_surface_del_struct->instance = p_engine->instance;
    p_surface_del_struct->surface = p_engine->surface;

    if(!deletion_stack_push(p_engine->p_main_del_stack, p_surface_del_struct, surface_destroy)) {
        LOG_ERROR("Failed to push deletion node");
        return false;
    }

    // Maybe move into vulkan_device_init?
    if(!vulkan_physical_device_init(p_engine->instance, p_engine->surface, &p_engine->physical_device)) {
        LOG_ERROR("Failed to create vulkan physical engine");
        return false;
    }

    if(!vulkan_device_init(p_engine->surface, p_engine->physical_device, &p_engine->device, &p_engine->queues)) {
        LOG_ERROR("Failed to create vulkan physical engine");
        return false;
    }

    if(!deletion_stack_push(p_engine->p_main_del_stack, p_engine->device, vulkan_device_destroy)) {
        LOG_ERROR("Failed to push deletion node");
        vulkan_device_destroy(p_engine->device);
        return false;
    }

    if(!vulkan_swapchain_init(
           p_engine->surface, p_engine->p_SDL_window, p_engine->physical_device, p_engine->device,
           &p_engine->vulkan_swapchain)) {
        LOG_ERROR("Failed to initiated vulkan swapchain");
        return false;
    }

    vulkan_swapchain_del_struct_t* p_swapchain_del_struct =
        (vulkan_swapchain_del_struct_t*)malloc(sizeof(vulkan_swapchain_del_struct_t));
    p_swapchain_del_struct->device = p_engine->device;
    p_swapchain_del_struct->vulkan_swapchain = p_engine->vulkan_swapchain;

    if(!deletion_stack_push(p_engine->p_main_del_stack, p_swapchain_del_struct, vulkan_swapchain_destroy)) {
        LOG_ERROR("Failed to push deletion node");
        vulkan_swapchain_destroy(p_swapchain_del_struct);
        return false;
    }

    if(!vulkan_image_create(
           p_engine->device, p_engine->physical_device, p_engine->window_extent.width, p_engine->window_extent.height,
           &p_engine->draw_image)) {
        LOG_ERROR("Failed to create image");
        return false;
    }

    allocated_image_del_strut_t* p_allocated_image_del_struct = (allocated_image_del_strut_t*)malloc(sizeof(allocated_image_del_strut_t));
    p_allocated_image_del_struct->device = p_engine->device;
    p_allocated_image_del_struct->allocated_image = p_engine->draw_image;
    if(!deletion_stack_push(p_engine->p_main_del_stack, p_allocated_image_del_struct, vulkan_image_destroy)) {
        LOG_ERROR("Failed to push onto deletion stack");
        vulkan_image_destroy(p_allocated_image_del_struct);
        return false;
    }

    // Create commands
    if(!vulkan_cmd_frame_init(p_engine->device, &p_engine->queues, p_engine->frames)) {
        LOG_ERROR("Failed to create frame commands");
        return false;
    }

    if(!vulkan_cmd_imm_init(p_engine->device, &p_engine->queues, &p_engine->imm_cmd_pool, &p_engine->imm_cmd_buffer)) {
        LOG_ERROR("Failed to create frame commands");
        return false;
    }
    
    for(int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        cmd_pool_del_struct_t* p_cmd_pool = (cmd_pool_del_struct_t*)malloc(sizeof(cmd_pool_del_struct_t));
        p_cmd_pool->device = p_engine->device;
        p_cmd_pool->cmd_pool = p_engine->frames[i].cmd_pool;
        if(!deletion_stack_push(p_engine->p_main_del_stack, p_cmd_pool, vulkan_cmd_pool_destroy)) {
            LOG_ERROR("Failed to push onto deletion stack");
            vulkan_cmd_pool_destroy(p_cmd_pool);
            return false;
        }
    }
    cmd_pool_del_struct_t* p_cmd_pool = (cmd_pool_del_struct_t*)malloc(sizeof(cmd_pool_del_struct_t));
    p_cmd_pool->device = p_engine->device;
    p_cmd_pool->cmd_pool = p_engine->imm_cmd_pool;
    if(!deletion_stack_push(p_engine->p_main_del_stack, p_cmd_pool, vulkan_cmd_pool_destroy)) {
            LOG_ERROR("Failed to push onto deletion stack");
            vulkan_cmd_pool_destroy(p_cmd_pool);
            return false;
        }

    // Create sync structures
    if(!create_sync_structs(p_engine)) {
        LOG_ERROR("Failed to create sync structures");
        return false;
    }

    // Create discriptors
    if(!create_descriptors(p_engine)) {
        LOG_ERROR("Failed to create descriptors");
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

    // Make sure the GPU has become idle before flushing deletion queue
    vkDeviceWaitIdle(p_engine->device);

    // Flush deletion queue
    if(!deletion_stack_flush(&p_engine->p_main_del_stack)) {
        LOG_ERROR("Failed to flush deletion queue");
        return false;
    }

    // Check that p_main_delq is NULL after flushing
    if(p_engine->p_main_del_stack != NULL) {
        LOG_ERROR("Failed to flush deletion queue");
        return false;
    }

    // Vulkan engine successfully destroyed
    LOG_INFO("Vulkan engine destroyed");
    return true;
}

static void surface_destroy(void* p_void_surface_del_struct) {
    LOG_DEBUG("Callback: surface_destroy");

    if(p_void_surface_del_struct == NULL) {
        LOG_ERROR("surface_destroy: sufrace_deL-struct is NULL");
        return;
    }

    // Cast pointer
    surface_del_struct_t* p_surface_del_struct = (surface_del_struct_t*)p_void_surface_del_struct;

    if(p_surface_del_struct->instance == NULL) {
        LOG_ERROR("surface_destroy: instance is NULL");
        return;
    }

    if(p_surface_del_struct->surface == NULL) {
        LOG_ERROR("surface_destroy: surface is NULL");
        return;
    }

    // Destroy surface
    vkDestroySurfaceKHR(p_surface_del_struct->instance, p_surface_del_struct->surface, VK_NULL_HANDLE);

    free(p_surface_del_struct);
    p_surface_del_struct = NULL;
    p_void_surface_del_struct = NULL;
}



static bool create_sync_structs(vulkan_engine_t* p_engine) {
    if(p_engine == NULL) {
        LOG_ERROR("create_sync_structs: p_engine is NULL");
        return false;
    }

    // Create sync structures

    // One fence to control when the GPU has finished rendering the frame and 2 semaphores to sync rendering with the
    // swapchain. We want the fence to start signaled so we can wait on it the first time.

    VkFenceCreateInfo fence_info = {0};

    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo sem_info = {0};

    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem_info.flags = 0;

    for(int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        if(vkCreateFence(p_engine->device, &fence_info, VK_NULL_HANDLE, &p_engine->frames[i].render_fence) !=
           VK_SUCCESS) {
            LOG_ERROR("Failed to create render fence");
            return false;
        }
        if(vkCreateSemaphore(p_engine->device, &sem_info, VK_NULL_HANDLE, &p_engine->frames[i].render_semaphore) !=
           VK_SUCCESS) {
            LOG_ERROR("Failed to create render semaphore");
            return false;
        }
        if(vkCreateSemaphore(p_engine->device, &sem_info, VK_NULL_HANDLE, &p_engine->frames[i].swapchain_semaphore) !=
           VK_SUCCESS) {
            LOG_ERROR("Failed to create swapchain semahpore");
            return false;
        }
    }

    if(vkCreateFence(p_engine->device, &fence_info, VK_NULL_HANDLE, &p_engine->imm_fence) != VK_SUCCESS) {
        LOG_ERROR("Failed to create immediate fence");
        return false;
    }

    if(!deletion_stack_push(p_engine->p_main_del_stack, p_engine, vkDestroyFence_wrapper)) {
        LOG_ERROR("Failed to queue deletion node");
        vkDestroyFence_wrapper(p_engine);
        return false;
    }

    if(!deletion_stack_push(p_engine->p_main_del_stack, p_engine, vkDestroyFence_Sem_wrapper)) {
        LOG_ERROR("Failed to queue deletion node");
        vkDestroyFence_Sem_wrapper(p_engine);
        return false;
    }

    LOG_INFO("Sync structures created");
    return true;
}

static void vkDestroyFence_wrapper(void* p_engine) {
    LOG_DEBUG("Callback: vkDestroyFence_wrapper");
    vkDestroyFence(((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->imm_fence, VK_NULL_HANDLE);
}

static void vkDestroyFence_Sem_wrapper(void* p_engine) {
    LOG_DEBUG("Callback: vkDestroyFence_Sem_wrapper");

    for(int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        LOG_DEBUG("    Destroying frame sync structs, index: %d", i);
        vkDestroyFence(
            ((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->frames[i].render_fence, VK_NULL_HANDLE);
        vkDestroySemaphore(
            ((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->frames[i].render_semaphore,
            VK_NULL_HANDLE);
        vkDestroySemaphore(
            ((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->frames[i].swapchain_semaphore,
            VK_NULL_HANDLE);
    }
}

static bool create_descriptors(vulkan_engine_t* p_engine) {
    if(p_engine == NULL) {
        LOG_ERROR("create_descriptors: p_engine is NULL");
        return false;
    }

    // Create a descriptor pool  that will hold 10 sets with 1 image each

    LOG_INFO("Descriptors created");
    return true;
}
