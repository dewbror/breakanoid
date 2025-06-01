#include <stdlib.h>
#include <vulkan/vulkan_core.h>

// this must be equal to that in imgui_impl_vulkan.h
//  #define IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE   (1)     // Minimum per atlas
//
//  #define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
//  #define CIMGUI_USE_SDL3
//  #define CIMGUI_USE_VULKAN
//  #include <cimgui.h>
//  #include <cimgui_impl.h>

#include "error/error.h"
#include "logger.h"
#include "util/deletion_stack.h"
#include "error/vulkan_error.h"
#include "imgui/imgui.h"

typedef struct imgui_del_s {
    VkDevice device;
    VkDescriptorPool imgui_pool;
} imgui_del_t;

static void imgui_deinit(void* p_void_imgui_del);

error_t imgui_init(deletion_stack_t* p_dstack, VkInstance instance, VkPhysicalDevice physical_device, VkDevice device,
    struct SDL_Window* p_window, VkQueue graphics_queue, VkFormat* p_swapchain_format)
{
    (void)p_dstack;
    (void)instance;
    (void)physical_device;
    (void)device;
    (void)p_window;
    (void)graphics_queue;
    (void)p_swapchain_format;

    // VkDescriptorPoolSize p_pool_sizes[] = {
    //     {VK_DESCRIPTOR_TYPE_SAMPLER,                1000}, // 1
    //     {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000}, // 2
    //     {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000}, // 3
    //     {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000}, // 4
    //     {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000}, // 5
    //     {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000}, // 6
    //     {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000}, // 7
    //     {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000}, // 8
    //     {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000}, // 9
    //     {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000}, // 10
    //     {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000}  // 11
    // };

    // VkDescriptorPoolCreateInfo pool_info = {0};
    // pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    // pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    // pool_info.maxSets = 1000;
    // pool_info.poolSizeCount = 11;
    // pool_info.pPoolSizes = p_pool_sizes;

    // VkDescriptorPool imgui_pool = {0};
    // VkResult result = vkCreateDescriptorPool(device, &pool_info, VK_NULL_HANDLE, &imgui_pool);
    // if(result != VK_SUCCESS)
    //     return error_init(ERR_SRC_VULKAN, VULKAN_ERR_CREATE_DESCRIPTOR_POOL, "Failed to create Vulkan descriptor
    //     pool");

    // // 2: Initialize imgui library

    // // This initializes the core structures of imgui
    // // ImGui::CreateContext();
    // igCreateContext(NULL);

    // // This initializes imgui for SDL
    // ImGui_ImplSDL3_InitForVulkan(p_window);

    // // This initializes imgui for Vulkan
    // ImGui_ImplVulkan_InitInfo init_info = {0};
    // init_info.Instance = instance;
    // init_info.PhysicalDevice = physical_device;
    // init_info.Device = device;
    // init_info.Queue = graphics_queue;
    // init_info.DescriptorPool = imgui_pool;
    // init_info.MinImageCount = 3;
    // init_info.ImageCount = 3;
    // init_info.UseDynamicRendering = true;

    // // Dynamic rendering parameters for imgui to use
    // // init_info.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    // init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    // init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    // init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = p_swapchain_format;

    // init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    // ImGui_ImplVulkan_Init(&init_info);

    // ImGui_ImplVulkan_CreateFontsTexture();

    // // CLEANUP
    // imgui_del_t* p_imgui_del = (imgui_del_t*)malloc(sizeof(imgui_del_t));
    // p_imgui_del->device = device;
    // p_imgui_del->imgui_pool = imgui_pool;

    // error_t err = deletion_stack_push(p_dstack, p_imgui_del, imgui_deinit);
    // if(err.code != 0) {
    //     imgui_deinit(p_imgui_del);
    //     return err;
    // }

    LOG_INFO("ImGui initiated");

    return SUCCESS;
}

static void imgui_deinit(void* p_void_imgui_del)
{
    (void)p_void_imgui_del;

    // Cast pointer
    // imgui_del_t* p_imgui_del = (imgui_del_t*)p_void_imgui_del;

    // ImGui_ImplVulkan_Shutdown();
    // vkDestroyDescriptorPool(p_imgui_del->device, p_imgui_del->imgui_pool, VK_NULL_HANDLE);

    // free(p_imgui_del);
    // p_imgui_del = NULL;
    // p_void_imgui_del = NULL;
}

error_t imgui_draw(VkCommandBuffer cmd, VkImageView tgt_img_view, VkExtent2D swapchain_extent)
{
    (void)cmd;
    (void)tgt_img_view;
    (void)swapchain_extent;

    // VkRenderingAttachmentInfo color_attachment = {0};
    // color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    // color_attachment.imageView = tgt_img_view;
    // color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // // color_attachment.clearValue = 0;

    // VkRenderingInfo render_info = {0};
    // render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    // render_info.colorAttachmentCount = 1;
    // render_info.pColorAttachments = &color_attachment;
    // render_info.renderArea.extent = swapchain_extent;
    // // renderInfo.flags                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // render_info.layerCount = 1;

    // vkCmdBeginRendering(cmd, &render_info);

    // vkCmdEndRendering(cmd);

    return SUCCESS;
}
