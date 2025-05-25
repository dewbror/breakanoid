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
static bool get_image_view(
    vulkan_engine_t* p_engine, VkImageView image_view, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags,
    uint32_t mip_levels);
/**
 * vkDestroyImageView wrapper for swapchin image view array
 *
 * \param[in] p_engine Pointer to the vulkan engine
 */
// static void vkDestroyImageView_array_wrapper(void* p_engine);

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

static bool create_image_views(vulkan_engine_t* p_engine) {
    if(p_engine == NULL) {
        LOG_ERROR("create_image_views: p_engine is NULL");
        return false;
    }

    // CREATE DRAW IMAGE

    p_engine->draw_image.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkExtent3D draw_image_extent = {p_engine->window_extent.width, p_engine->window_extent.height, 1};
    p_engine->draw_image.extent = draw_image_extent;

    VkImageUsageFlags draw_image_usage = 0;

    draw_image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    draw_image_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    draw_image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    draw_image_usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    draw_image_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo img_info = {0};

    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.extent = p_engine->draw_image.extent;
    img_info.extent.depth = 1;
    img_info.mipLevels = 1;
    img_info.arrayLayers = 1;
    img_info.format = p_engine->draw_image.format;      // Or your needed format
    img_info.tiling = VK_IMAGE_TILING_OPTIMAL;          // Usually optimal for GPU use
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // = 0 default value
    img_info.usage = draw_image_usage;                  // Example
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // = 0 default value

    if(vkCreateImage(p_engine->device, &img_info, VK_NULL_HANDLE, &p_engine->draw_image.image) != VK_SUCCESS) {
        LOG_ERROR("Failed to create draw image");
        return false;
    }

    // For the draw image, we want to allocate it on the GPU local memory
    // ALLOCATE MEMORY ON THE GPU

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(p_engine->device, p_engine->draw_image.image, &mem_req);

    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_req.size;

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

    img_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    img_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    img_view_info.image = p_engine->draw_image.image;
    img_view_info.format = p_engine->draw_image.format;
    img_view_info.subresourceRange.baseMipLevel = 0;
    img_view_info.subresourceRange.levelCount = 1;
    img_view_info.subresourceRange.baseArrayLayer = 0;
    img_view_info.subresourceRange.layerCount = 1;
    img_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if(vkCreateImageView(p_engine->device, &img_view_info, VK_NULL_HANDLE, &p_engine->draw_image.image_view) !=
       VK_SUCCESS) {
        LOG_ERROR("Failed to create draw image view");
        return false;
    }

    // ADD DESTRUCTION OF DRAW IMAGE AND DRAW IMAGE VIEW TO DELETION QUEUE

    if(!deletion_stack_push(p_engine->p_main_del_stack, p_engine, vkDestroyImage_FreeMemory_wrapper)) {
        LOG_ERROR("Failed to queue deletion node");
        vkDestroyImage_FreeMemory_wrapper(p_engine);
        return false;
    }

    if(!deletion_stack_push(p_engine->p_main_del_stack, p_engine, vkDestroyImageView_wrapper)) {
        LOG_ERROR("Failed to queue deletion node");
        vkDestroyImageView_wrapper(p_engine);
        return false;
    }

    LOG_INFO("Image views created");
    return true;
}

static bool get_image_view(
    vulkan_engine_t* p_engine, VkImageView image_view, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags,
    uint32_t mip_levels) {
    // TODO: Sanatize inputs

    // Fill image view create info
    VkImageViewCreateInfo view_info = {0};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;

    // The viewType and format fields specify how the image data should be interpreted. The viewType parameter
    // allows you to treat images as 1D textures, 2D textures, 3D textures and cube maps.
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;

    // The components field allows you to swizzle the color channels around. For example, you can map all of the
    // channels to the red channel for a monochrome texture. You can also map constant values of 0 and 1 to a
    // channel. In our case we’ll stick to the default mapping.
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // The subresourceRange field describes what the image’s purpose is and which part of the image should be
    // accessed. Our images will be used as color targets without any mipmapping levels or multiple layers.
    view_info.subresourceRange.aspectMask = aspect_flags;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = mip_levels;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    // Create image view
    if(vkCreateImageView(p_engine->device, &view_info, VK_NULL_HANDLE, &image_view) != VK_SUCCESS) {
        LOG_ERROR("Failed to create image view");
        return false;
    }

    LOG_DEBUG("Image view created");
    return true;
}

static void vkDestroyImage_FreeMemory_wrapper(void* p_engine) {
    LOG_DEBUG("Callback: vkDestroyImage_FreeMemory_wrapper");
    vkDestroyImage(
        ((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->draw_image.image,
        VK_NULL_HANDLE); // Destroy the VkImage
    vkFreeMemory(
        ((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->draw_image.mem,
        VK_NULL_HANDLE); // Free the device memory
}

static void vkDestroyImageView_wrapper(void* p_engine) {
    LOG_DEBUG("Callback: vkDestroyImageView_wrapper");
    vkDestroyImageView(
        ((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->draw_image.image_view, VK_NULL_HANDLE);
}

static bool create_commands(vulkan_engine_t* p_engine) {
    if(p_engine == NULL) {
        LOG_ERROR("create_commands: p_engine is NULL");
        return false;
    }

    // Create a command pool for commands submitted to the graphics queue
    // We also want the pool to allow for resetting of individual command buffers

    VkCommandPoolCreateInfo cmd_pool_info = {0};

    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_info.queueFamilyIndex = p_engine->queues.graphics_index;

    // Allocate the default command buffer that we will use for rendering
    VkCommandBufferAllocateInfo render_cmd_alloc_info = {0};

    render_cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    render_cmd_alloc_info.commandBufferCount = 1;
    render_cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
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

    imm_cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    imm_cmd_alloc_info.commandBufferCount = 1;
    imm_cmd_alloc_info.commandPool = p_engine->imm_cmd_pool;
    imm_cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    if(vkAllocateCommandBuffers(p_engine->device, &imm_cmd_alloc_info, &p_engine->imm_cmd_buffer) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate immediate command buffer");
        return false;
    }

    if(!deletion_stack_push(p_engine->p_main_del_stack, p_engine, vkDestroyCommandPool_wrapper)) {
        LOG_ERROR("Failed to queue deletion node");
        vkDestroyCommandPool_wrapper(p_engine);
        return false;
    }

    if(!deletion_stack_push(p_engine->p_main_del_stack, p_engine, vkDestroyCommandPool_array_wrapper)) {
        LOG_ERROR("Failed to queue deletion node");
        vkDestroyCommandPool_array_wrapper(p_engine);
        return false;
    }

    LOG_INFO("Command structures created");
    return true;
}

static void vkDestroyCommandPool_wrapper(void* p_engine) {
    LOG_DEBUG("Callback: vkDestroyCommandPool_wrapper");
    vkDestroyCommandPool(
        ((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->imm_cmd_pool, VK_NULL_HANDLE);
}

static void vkDestroyCommandPool_array_wrapper(void* p_engine) {
    LOG_DEBUG("Callback: vkDestroyCommandPool_array_wrapper");
    for(int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        LOG_DEBUG("    Destroying frame command pool, index: %d", i);
        vkDestroyCommandPool(
            ((vulkan_engine_t*)p_engine)->device, ((vulkan_engine_t*)p_engine)->frames[i].cmd_pool, VK_NULL_HANDLE);
    }
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
