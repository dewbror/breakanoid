#include <stdbool.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#include "logger.h"
#include "vulkan/vulkan_types.h"
#include "vulkan/vulkan_image.h"

bool vulkan_image_create(VkDevice device, VkPhysicalDevice physical_device, uint32_t width, uint32_t height, allocated_image_t* p_allocated_image) {
    if(device == NULL) {
        LOG_ERROR("vulkan_image_create: device is NULL");
        return false;
    }

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
    img_info.format = p_allocated_image->format;      // Or your needed format
    img_info.tiling = VK_IMAGE_TILING_OPTIMAL;          // Usually optimal for GPU use
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // = 0 default value
    img_info.usage = draw_image_usage;                  // Example
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // = 0 default value

    if(vkCreateImage(device, &img_info, VK_NULL_HANDLE, &p_allocated_image->image) != VK_SUCCESS) {
        LOG_ERROR("Failed to create draw image");
        return false;
    }

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

    if(vkCreateImageView(device, &img_view_info, VK_NULL_HANDLE, &p_allocated_image->image_view) !=
       VK_SUCCESS) {
        LOG_ERROR("Failed to create draw image view");
        return false;
    }

    LOG_INFO("Image views created");
    return true;
}

// static bool get_image_view(
//     vulkan_engine_t* p_engine, VkImageView image_view, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags,
//     uint32_t mip_levels) {
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

void vulkan_image_destroy(void* p_void_allocated_image_del_struct) {
    LOG_DEBUG("Callback: vulkan_destroy_image");

    // Cast pointer
    allocated_image_del_strut_t* p_allocated_image_del_struct = (allocated_image_del_strut_t*)p_void_allocated_image_del_struct;

    vkDestroyImage(p_allocated_image_del_struct->device, p_allocated_image_del_struct->allocated_image.image, VK_NULL_HANDLE);
    vkFreeMemory(p_allocated_image_del_struct->device, p_allocated_image_del_struct->allocated_image.mem, VK_NULL_HANDLE);
    vkDestroyImageView(p_allocated_image_del_struct->device, p_allocated_image_del_struct->allocated_image.image_view, VK_NULL_HANDLE);

    free(p_allocated_image_del_struct);
    p_allocated_image_del_struct = NULL;
    p_void_allocated_image_del_struct = NULL;
}
