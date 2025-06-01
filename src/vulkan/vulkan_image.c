#include <stdbool.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#include "error/error.h"
#include "error/vulkan_error.h"
#include "logger.h"
#include "util/deletion_stack.h"
#include "vulkan/vulkan_types.h"
#include "vulkan/vulkan_image.h"

/**
 * Struct used for deleting an image
 */
typedef struct alloc_img_del_s {
    VkDevice device;
    allocated_image_t allocated_image;
} alloc_img_del_t;

/**
 * Deinitialize a vulkan image.
 */
void vulkan_image_deinit(void* p_void_allocated_image_del_struct);

/**
 * \brief blablabla
 */
static VkImageSubresourceRange img_subresource_Range(VkImageAspectFlags aspect_mask);

error_t vulkan_image_create(deletion_stack_t* p_dstack, VkDevice device, VkPhysicalDevice physical_device,
    uint32_t width, uint32_t height, allocated_image_t* p_allocated_image)
{
    if(device == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: device is NULL", __func__);

    if(physical_device == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: physical_device is NULL", __func__);

    // CREATE DRAW IMAGE

    p_allocated_image->format = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkExtent3D draw_image_extent = {width, height, 1};
    p_allocated_image->extent = draw_image_extent;

    VkImageUsageFlags draw_image_usage = 0;

    draw_image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    draw_image_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    draw_image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    draw_image_usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    draw_image_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo img_info = {0};

    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.extent = p_allocated_image->extent;
    img_info.extent.depth = 1;
    img_info.mipLevels = 1;
    img_info.arrayLayers = 1;
    img_info.format = p_allocated_image->format;        // Or your needed format
    img_info.tiling = VK_IMAGE_TILING_OPTIMAL;          // Usually optimal for GPU use
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // = 0 default value
    img_info.usage = draw_image_usage;                  // Example
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // = 0 default value

    if(vkCreateImage(device, &img_info, VK_NULL_HANDLE, &p_allocated_image->image) != VK_SUCCESS)
        return error_init(ERR_SRC_VULKAN, VULKAN_ERR_IMAGE, "Failed to create draw image");

    // For the draw image, we want to allocate it on the GPU local memory
    // ALLOCATE MEMORY ON THE GPU

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(device, p_allocated_image->image, &mem_req);

    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_req.size;

    // Find a device local memory type
    uint32_t mem_type_index = 0;
    VkPhysicalDeviceMemoryProperties mem_prop;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_prop);

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
    vkAllocateMemory(device, &alloc_info, VK_NULL_HANDLE, &p_allocated_image->mem);
    vkBindImageMemory(device, p_allocated_image->image, p_allocated_image->mem, 0);

    // CREATE DRAW IMAGE VIEW

    VkImageViewCreateInfo img_view_info = {0};

    img_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    img_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    img_view_info.image = p_allocated_image->image;
    img_view_info.format = p_allocated_image->format;
    img_view_info.subresourceRange.baseMipLevel = 0;
    img_view_info.subresourceRange.levelCount = 1;
    img_view_info.subresourceRange.baseArrayLayer = 0;
    img_view_info.subresourceRange.layerCount = 1;
    img_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if(vkCreateImageView(device, &img_view_info, VK_NULL_HANDLE, &p_allocated_image->image_view) != VK_SUCCESS)
        return error_init(ERR_SRC_VULKAN, VULKAN_ERR_IMAGE_VIEW, "Failed to create draw image view");

    // Add cleanup
    alloc_img_del_t* p_img_del = (alloc_img_del_t*)malloc(sizeof(alloc_img_del_t));
    p_img_del->device = device;
    p_img_del->allocated_image = *p_allocated_image;

    error_t err = deletion_stack_push(p_dstack, p_img_del, vulkan_image_deinit);
    if(err.code != 0) {
        vulkan_image_deinit(p_img_del);
        return err;
    }

    LOG_DEBUG("%s: Successful", __func__);

    return SUCCESS;
}

// static bool get_image_view(
//     vulkan_engine_t* p_engine, VkImageView image_view, VkImage image, VkFormat format, VkImageAspectFlags
//     aspect_flags, uint32_t mip_levels) {
//     // TODO: Sanatize inputs
//
//     // Fill image view create info
//     VkImageViewCreateInfo view_info = {0};
//     view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//     view_info.image = image;
//
//     // The viewType and format fields specify how the image data should be interpreted. The viewType parameter
//     // allows you to treat images as 1D textures, 2D textures, 3D textures and cube maps.
//     view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
//     view_info.format = format;
//
//     // The components field allows you to swizzle the color channels around. For example, you can map all of the
//     // channels to the red channel for a monochrome texture. You can also map constant values of 0 and 1 to a
//     // channel. In our case we’ll stick to the default mapping.
//     view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
//     view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
//     view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
//     view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
//
//     // The subresourceRange field describes what the image’s purpose is and which part of the image should be
//     // accessed. Our images will be used as color targets without any mipmapping levels or multiple layers.
//     view_info.subresourceRange.aspectMask = aspect_flags;
//     view_info.subresourceRange.baseMipLevel = 0;
//     view_info.subresourceRange.levelCount = mip_levels;
//     view_info.subresourceRange.baseArrayLayer = 0;
//     view_info.subresourceRange.layerCount = 1;
//
//     // Create image view
//     if(vkCreateImageView(p_engine->device, &view_info, VK_NULL_HANDLE, &image_view) != VK_SUCCESS) {
//         LOG_ERROR("Failed to create image view");
//         return false;
//     }
//
//     LOG_DEBUG("Image view created");
//     return true;
// }

void vulkan_image_deinit(void* p_void_img_del)
{
    LOG_DEBUG("Callback: vulkan_destroy_image");

    if(p_void_img_del == NULL) {
        LOG_ERROR("%s: p_void_allocated_image_del_struct is NULL", __func__);
        return;
    }

    // NULL check struct content

    // Cast pointer
    alloc_img_del_t* p_img_del = (alloc_img_del_t*)p_void_img_del;

    vkDestroyImage(p_img_del->device, p_img_del->allocated_image.image, VK_NULL_HANDLE);
    vkFreeMemory(p_img_del->device, p_img_del->allocated_image.mem, VK_NULL_HANDLE);
    vkDestroyImageView(p_img_del->device, p_img_del->allocated_image.image_view, VK_NULL_HANDLE);

    free(p_img_del);
    p_img_del = NULL;
    p_void_img_del = NULL;
}

void vulkan_image_transition(VkCommandBuffer cmd, VkImage img, VkImageLayout old_layout, VkImageLayout new_layout)
{
    // VkImageMemoryBarrier2 contains the information for a given image barrier. On here, is where we set the old and
    // new layouts. In the StageMask, we are doing ALL_COMMANDS. This is inefficient, as it will stall the GPU pipeline
    // a bit. For our needs, its going to be fine as we are only going to do a few transitions per frame. If you are
    // doing many transitions per frame as part of a post-process chain, you want to avoid doing this, and instead use
    // StageMasks more accurate to what you are doing.

    // AllCommands stage mask on the barrier means that the barrier will stop the gpu commands completely when it
    // arrives at the barrier. By using more finegrained stage masks, its possible to overlap the GPU pipeline across
    // the barrier a bit. AccessMask is similar, it controls how the barrier stops different parts of the GPU. we are
    // going to use VK_ACCESS_2_MEMORY_WRITE_BIT for our source, and add VK_ACCESS_2_MEMORY_READ_BIT to our destination.
    // Those are generic options that will be fine.

    // If you want to read about what would be the optimal way of using pipeline barriers for different use cases, you
    // can find a great reference in here:
    //  - https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
    // This layout transition is going to work just fine for the whole tutorial, but if you want, you can add more
    // complicated transition functions that are more accurate/lightweight.
    VkImageMemoryBarrier2 img_barrier2 = {0};
    img_barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    img_barrier2.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    switch(old_layout) { // NOLINT
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        img_barrier2.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        break;
    default:
        img_barrier2.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        break;
    }

    img_barrier2.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    switch(new_layout) { // NOLINT
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        img_barrier2.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        img_barrier2.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        img_barrier2.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
        img_barrier2.dstAccessMask = 0;
        break;
    default:
        img_barrier2.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
        break;
    }

    img_barrier2.oldLayout = old_layout;
    img_barrier2.newLayout = new_layout;

    VkImageAspectFlags aspect_mask = (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ?
        VK_IMAGE_ASPECT_DEPTH_BIT :
        VK_IMAGE_ASPECT_COLOR_BIT;
    img_barrier2.subresourceRange = img_subresource_Range(aspect_mask);
    img_barrier2.image = img;

    VkDependencyInfo dep_info = {0};
    dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers = &img_barrier2;

    vkCmdPipelineBarrier2(cmd, &dep_info);
}

static VkImageSubresourceRange img_subresource_Range(VkImageAspectFlags aspect_mask)
{
    VkImageSubresourceRange sub_image = {0};
    sub_image.aspectMask = aspect_mask;
    sub_image.baseMipLevel = 0;
    sub_image.levelCount = VK_REMAINING_MIP_LEVELS;
    sub_image.baseArrayLayer = 0;
    sub_image.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return sub_image;
}

void vulkan_image_copy_image_to_image(VkCommandBuffer cmd, VkImage src_img, VkImage dst_img, VkExtent2D src_ext,
    VkExtent2D dst_ext)
{
    VkImageBlit2 blit_region = {0};
    blit_region.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;

    blit_region.srcOffsets[1].x = (int32_t)src_ext.width;
    blit_region.srcOffsets[1].y = (int32_t)src_ext.height;
    blit_region.srcOffsets[1].z = 1;

    blit_region.dstOffsets[1].x = (int32_t)dst_ext.width;
    blit_region.dstOffsets[1].y = (int32_t)dst_ext.height;
    blit_region.dstOffsets[1].z = 1;

    blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.srcSubresource.baseArrayLayer = 0;
    blit_region.srcSubresource.layerCount = 1;
    blit_region.srcSubresource.mipLevel = 0;

    blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.dstSubresource.baseArrayLayer = 0;
    blit_region.dstSubresource.layerCount = 1;
    blit_region.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blit_info = {0};
    blit_info.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    blit_info.dstImage = dst_img;
    blit_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blit_info.srcImage = src_img;
    blit_info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blit_info.filter = VK_FILTER_LINEAR;
    blit_info.regionCount = 1;
    blit_info.pRegions = &blit_region;

    vkCmdBlitImage2(cmd, &blit_info);
}
