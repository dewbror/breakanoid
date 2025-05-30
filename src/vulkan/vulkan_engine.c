#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan_core.h>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_vulkan.h>

#include "imgui/imgui.h"
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
#include "vulkan/vulkan_descriptor.h"
#include "vulkan/vulkan_pipeline.h"
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
 */
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

static void draw_background(VkCommandBuffer cmd, VkPipeline pipeline, VkPipelineLayout pipeline_layout,
    VkDescriptorSet desc_set, VkExtent2D draw_extent);

error_t vulkan_engine_init(vulkan_engine_t* p_engine) {
    // Check if p_engine is NULL
    if(p_engine == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: p_engine is NULL", __func__);

    // Set window extent, this will be performed from some settings file in the future.
    p_engine->window_extent.height = HEIGHT;
    p_engine->window_extent.width = WIDTH;

    p_engine->frame_count = 0;

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

    err = vulkan_pipeline_init(p_engine->device, p_engine->physical_device, p_engine->window_extent,
        &p_engine->draw_img_desc_layout, &p_engine->gradient_pipline_layout, &p_engine->gradient_pipline);
    if(err.code != 0)
        return err;

    vulkan_pipeline_del_t* p_pipeline_del = (vulkan_pipeline_del_t*)malloc(sizeof(vulkan_pipeline_del_t));
    p_pipeline_del->device = p_engine->device;
    p_pipeline_del->layout = p_engine->gradient_pipline_layout;
    p_pipeline_del->pipeline = p_engine->gradient_pipline;

    err = deletion_stack_push(p_engine->p_main_del_stack, p_pipeline_del, vulkan_pipeline_destroy);
    if(err.code != 0) {
        vulkan_pipeline_destroy(p_pipeline_del);
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

void vulkan_engine_render_and_present_frame(vulkan_engine_t* p_engine) {
    // The the current frame
    frame_data_t frame = p_engine->p_frames[p_engine->frame_count % FRAMES_IN_FLIGHT];

    // Wait until device has finished rendering the last frame. TIMEOUT of UINT64_MAX nanoseconds
    if(vkWaitForFences(p_engine->device, 1, &frame.render_fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
        return;

    // Flush the current frames deletion stack
    // deletion_stack_flush(&frame.p_del_stack);

    // Reset render fence
    if(vkResetFences(p_engine->device, 1, &frame.render_fence) != VK_SUCCESS)
        return;

    // Request image from the swapchain
    uint32_t index = 0;
    if(vkAcquireNextImageKHR(p_engine->device, p_engine->vulkan_swapchain.swapchain, UINT64_MAX,
           frame.swapchain_semaphore, VK_NULL_HANDLE, &index) != VK_SUCCESS)
        return;

    // Recreate swapchain if Failed

    // Reset cmd buffer
    if(vkResetCommandBuffer(frame.cmd, 0) != VK_SUCCESS)
        return;

    // LOG_TRACE("draw_image.extent.width: %u", p_engine->draw_image.extent.width);
    // LOG_TRACE("draw_image.extent.height: %u", p_engine->draw_image.extent.height);

    p_engine->draw_extent.width = p_engine->draw_image.extent.width;
    p_engine->draw_extent.height = p_engine->draw_image.extent.height;

    // Start cmd buffer recording
    VkCommandBufferBeginInfo cmd_begin_info = {0};
    cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if(vkBeginCommandBuffer(frame.cmd, &cmd_begin_info) != VK_SUCCESS)
        return;

    // Transition our main draw image into general layout so we can write into it, we will overwrite it all so we dont
    // need to care about what the previous layout was
    vulkan_image_transition(frame.cmd, p_engine->draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    draw_background(frame.cmd, p_engine->gradient_pipline, p_engine->gradient_pipline_layout, p_engine->draw_img_desc,
        p_engine->draw_extent);

    // Transition the draw image and the swapchain image to the correct transfer layouts
    vulkan_image_transition(
        frame.cmd, p_engine->draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vulkan_image_transition(frame.cmd, p_engine->vulkan_swapchain.p_images[index], VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Execute a blit from the draw image to the swapchain image
    vulkan_image_copy_image_to_image(frame.cmd, p_engine->draw_image.image, p_engine->vulkan_swapchain.p_images[index],
        p_engine->draw_extent, p_engine->vulkan_swapchain.extent);

    // Set swapchin image layout to Color Attachment Optimal so imgui can render into it
    // vulkan_image_transition(cmd, p_engine->vulkan_swapchain.p_images[index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // draw_imgui

    vulkan_image_transition(frame.cmd, p_engine->vulkan_swapchain.p_images[index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // End command buffer
    if(vkEndCommandBuffer(frame.cmd) != VK_SUCCESS)
        return;

    // Prepare the submission to the queue. We wat to wait on the _presentSemaphore, as that semaphore is signaled when
    // the swapchain is ready. We will signal the _renderSemaphore, to signal that rendering has finished.
    VkCommandBufferSubmitInfo cmd_info = vulkan_cmd_get_buffer_submit_info(frame.cmd);

    VkSemaphoreSubmitInfo wait_info =
        vulkan_sync_get_sem_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, frame.swapchain_semaphore);

    VkSemaphoreSubmitInfo signal_info =
        vulkan_sync_get_sem_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, frame.render_semaphore);

    VkSubmitInfo2 submit_info2 = vulkan_cmd_get_submit_info2(&cmd_info, &signal_info, &wait_info);

    // Submit command buffer to the queue and execute it.
    // _renderFence will now block until the graphics commands finish execution.
    if(vkQueueSubmit2(p_engine->queues.graphics, 1, &submit_info2, frame.render_fence) != VK_SUCCESS)
        return;

    // Prepare present
    // This will present the image we just rendered onto the SDL window.
    // We want to wait on the _renderSemaphore for that, as its necessary that drawing commands have finished before the
    // image is presented.
    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pSwapchains = &p_engine->vulkan_swapchain.swapchain;
    present_info.swapchainCount = 1;
    present_info.pWaitSemaphores = &frame.render_semaphore;
    present_info.waitSemaphoreCount = 1;
    present_info.pImageIndices = &index;

    // Present rendered image
    if(vkQueuePresentKHR(p_engine->queues.present, &present_info) != VK_SUCCESS)
        return;

    // Increase the number of frames drawn
    ++p_engine->frame_count; // Watch out for overflow!!
}

static void draw_background(VkCommandBuffer cmd, VkPipeline pipeline, VkPipelineLayout pipeline_layout,
    VkDescriptorSet desc_set, VkExtent2D draw_extent) {
    // Bind the gradient drawing compute pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

    // Bind the descriptor set containting the draw image for the compute pipeline
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &desc_set, 0, VK_NULL_HANDLE);

    // Guard against range overflow when casting from double to uint32
    uint32_t group_count_x = 0;
    uint32_t group_count_y = 0;
    double g_count = 0;

    g_count = ceil(draw_extent.width / 32.0);
    if(g_count < 0 || g_count > (double)UINT32_MAX)
        return;
    group_count_x = (uint32_t)g_count;

    g_count = ceil(draw_extent.height / 32.0);
    if(g_count < 0 || g_count > (double)UINT32_MAX)
        return;
    group_count_y = (uint32_t)g_count;

    // LOG_TRACE("group_count_x: %u", group_count_x);
    // LOG_TRACE("group_count_y: %u", group_count_y);

    // Dispatch compute pipeline
    vkCmdDispatch(cmd, group_count_x, group_count_y, 1);
}
