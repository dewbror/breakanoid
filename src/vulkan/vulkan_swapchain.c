#include <errno.h>
#include <stdbool.h>

#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include <SDL3/SDL_video.h>

#include "error/error.h"
#include "error/vulkan_error.h"
#include "logger.h"
#include "vulkan/vulkan_types.h"
#include "vulkan/vulkan_device.h"
#include "vulkan/vulkan_swapchain.h"

/**
 * Choose the swapchain surface format.
 *
 * \param[in] p_formats Array of supported swapchain surface formats.
 * \param[in] formats_count Number of elements in the formats array.
 *
 * \return The choosen swapchain surface format.
 */
static VkSurfaceFormatKHR choose_swapchain_surface_format(VkSurfaceFormatKHR* p_formats, size_t formats_count);

/**
 * Choose the swapchain present mode.
 *
 * \param[in] p_present_modes Array of supported swapchain present modes.
 * \param[in] present_modes_count Number of elements in the present modes array.
 *
 * \return The chosen present mode.
 */
static VkPresentModeKHR choose_swapchain_present_mode(VkPresentModeKHR* p_present_modes, size_t present_modes_count);

/**
 * Choose the swapchain extent. This is chosen to the pixel size given by SDL_GetWindowSizeInPixels. This is fairly
 * straight forward except for when the display is a "high pixel density display" in which case the pixel size can be
 * larger than the screen coordinates. SDL_GetWindowSizeInPixels should get the actual pixel size and not the screen
 * coordiantes.
 *
 * \param[in] p_window Pointer to the SDL window.
 * \param[in] capabilities The swapchain surface capabilities.
 *
 * \return The chosen swapchain extent. If the SDL_GetWindowSizeInPixels the extent will have 0 width and height.
 */
static VkExtent2D choose_swapchain_extent(SDL_Window* p_window, VkSurfaceCapabilitiesKHR capabilities);

error_t vulkan_swapchain_init(
    VkDevice device, VkPhysicalDevice physical_device, VkSurfaceKHR surface, SDL_Window* p_window,
    vulkan_swapchain_t* p_vulkan_swapchain
) {

    if(device == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: device is NULL", __func__);

    if(physical_device == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: physical_device is NULL", __func__);

    if(surface == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: surface is NULL", __func__);

    if(p_window == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: p_window is NULL", __func__);

    if(p_vulkan_swapchain == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: p_vulkan_swapchain is NULL", __func__);

    // Swapchain support has already been checked but we run this function again to retrieve the swapchain support
    // details (the surface formats and present modes)
    swapchain_support_details_t swapchain_support = {0};
    if(!vulkan_device_get_swapchain_support(surface, physical_device, &swapchain_support))
        return error_init(ERR_SRC_CORE, ERR_TEMP, "Swapchain not supported by device");

    // Choose which surface formats we want to use
    VkSurfaceFormatKHR surface_format =
        choose_swapchain_surface_format(swapchain_support.formats, swapchain_support.formats_count);

    // Choose which present modes we want to use
    VkPresentModeKHR present_mode =
        choose_swapchain_present_mode(swapchain_support.present_modes, swapchain_support.present_modes_count);

    // Choose our swapchain extent
    VkExtent2D extent = choose_swapchain_extent(p_window, swapchain_support.capabilities);

    // Check that the extent is non-zero
    if(extent.height == 0 && extent.width == 0)
        return error_init(ERR_SRC_CORE, ERR_WINDOW_EXTENT, "The swapchain extent is zero in one/both dimensions");

    // Aside from these properties we also have to decide how many images we would like to have in the swap chain.
    // The implementation specifies the minimum number that it requires to function. However, simply sticking to
    // this minimum means that we may sometimes have to wait on the driver to complete internal operations before we
    // can acquire another image to render to. Therefore it is recommended to request at least one more image than
    // the minimum.
    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;

    // We should also make sure to not exceed the maximum number of images while doing this, where 0 is a special
    // value that means that there is no maximum.
    if(swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount)
        image_count = swapchain_support.capabilities.maxImageCount;

    LOG_DEBUG("Minimum number of swapchain images to create: %u", image_count);

    // Create the swapchain
    VkSwapchainCreateInfoKHR create_swapchain_info = {0};

    create_swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    // Here we specify which surface the swapchain is tied to.
    create_swapchain_info.surface = surface;
    // After specifying which surface the swapchain should be tied to, the details of the swapchain images are
    // specified.
    create_swapchain_info.minImageCount = image_count;
    create_swapchain_info.imageFormat = surface_format.format;
    create_swapchain_info.imageColorSpace = surface_format.colorSpace;
    create_swapchain_info.imageExtent = extent;
    create_swapchain_info.imageArrayLayers = 1;
    // create_swapchain_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // I am attempting to follow the cppvk13 tutorial where the swapchain is first created with this bit
    // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT is the default for vk-bootstrap
    create_swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    // The imageArrayLayers specifies the amount of layers each image consists of. This is always 1 unless you are
    // developing a stereoscopic 3D application. The imageUsage bit field specifies what kind of operations we’ll
    // use the images in the swap chain for. In this tutorial we’re going to render directly to them, which means
    // that they’re used as color attachment. It is also possible that you’ll render images to a separate image
    // first to perform operations like post-processing. In that case you may use a value like
    // VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use a memory operation to transfer the rendered image to a swap
    // chain image.
    queue_family_data_t queues = {0};
    vulkan_device_get_queue_families(surface, physical_device, &queues);

    // If the queue families are not the same then they need to share the swapchain, if they are then theres no problem.
    uint32_t q_fam_indices_array[] = {queues.graphics_index, queues.present_index};
    if(queues.graphics_index != queues.present_index) {
        create_swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_swapchain_info.queueFamilyIndexCount = 2;
        create_swapchain_info.pQueueFamilyIndices = q_fam_indices_array;
    } else {
        create_swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_swapchain_info.queueFamilyIndexCount = 0;            // Optional
        create_swapchain_info.pQueueFamilyIndices = VK_NULL_HANDLE; // Optional
    }

    // We can specify that a certain transform should be applied to images in the swap chain if it is supported
    // (supportedTransforms in capabilities), like a 90 degree clockwise rotation or horizontal flip. To specify
    // that you do not want any transformation, simply specify the current transformation.
    create_swapchain_info.preTransform = swapchain_support.capabilities.currentTransform;

    // The compositeAlpha field specifies if the alpha channel should be used for blending with other windows in the
    // window system. You’ll almost always want to simply ignore the alpha channel, hence
    // VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR.
    create_swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_swapchain_info.presentMode = present_mode;

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
    if(vkCreateSwapchainKHR(device, &create_swapchain_info, VK_NULL_HANDLE, &p_vulkan_swapchain->swapchain) !=
       VK_SUCCESS)
        return error_init(ERR_SRC_VULKAN, VULKAN_ERR_SWAPCHAIN, "Failed to create swapchain");

    LOG_INFO("Swapchain created");

    // The swap chain has been created now, so all that remains is retrieving the handles of the VkImages in it.
    // We’ll reference these during rendering operations in later chapters. The images were created by the
    // implementation for the swapchain and they will be automatically cleaned up once the swap chain has been
    // destroyed, therefore we don’t need to add any cleanup code. Remember that we only specified a minimum number
    // of images in the swap chain, so the implementation is allowed to create a swapchain with more. That’s why
    // we’ll first query the final number of images with vkGetSwapchainImagesKHR, then resize the container and
    // finally call it again to retrieve the handles.
    vkGetSwapchainImagesKHR(device, p_vulkan_swapchain->swapchain, &image_count, VK_NULL_HANDLE);

    LOG_DEBUG("Number of swapchain images created: %u", image_count);

    // Allocate array to hole swapchain images
    p_vulkan_swapchain->p_images = (VkImage*)malloc(image_count * sizeof(VkImage));
    p_vulkan_swapchain->images_count = image_count;

    // Get swapchain images
    vkGetSwapchainImagesKHR(device, p_vulkan_swapchain->swapchain, &image_count, p_vulkan_swapchain->p_images);

    // Store the format and extent we’ve chosen for the swap chain images
    p_vulkan_swapchain->format = surface_format.format;
    p_vulkan_swapchain->extent = extent;

    // These are allocated in get swapchain support
    free(swapchain_support.formats);
    swapchain_support.formats = NULL;

    free(swapchain_support.present_modes);
    swapchain_support.present_modes = NULL;

    // Allocate array to hold image views
    p_vulkan_swapchain->p_image_views = (VkImageView*)malloc(p_vulkan_swapchain->images_count * sizeof(VkImageView));
    if(p_vulkan_swapchain->p_image_views == NULL)
        return error_init(
            ERR_SRC_CORE, ERR_MALLOC, "%s: Failed to allocate memory of size %lu", __func__,
            p_vulkan_swapchain->images_count * sizeof(VkImageView)
        );

    // create image views
    LOG_DEBUG("Creating swapchain image views");

    for(uint32_t i = 0; i < p_vulkan_swapchain->images_count; ++i) {
        LOG_DEBUG("   index: %d", i);
        // get_image_view(p_engine, p_engine->swapchain_images.p_image_views[i], p_engine->swapchain_images.p_images[i],
        //                  p_engine->swapchain_image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        VkImageViewCreateInfo view_info = {0};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = p_vulkan_swapchain->p_images[i];

        // The viewType and format fields specify how the image data should be interpreted. The viewType parameter
        // allows you to treat images as 1D textures, 2D textures, 3D textures and cube maps.
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = p_vulkan_swapchain->format;

        // The components field allows you to swizzle the color channels around. For example, you can map all of the
        // channels to the red channel for a monochrome texture. You can also map constant values of 0 and 1 to a
        // channel. In our case we’ll stick to the default mapping.
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // The subresourceRange field describes what the image’s purpose is and which part of the image should be
        // accessed. Our images will be used as color targets without any mipmapping levels or multiple layers.
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        // Create image view
        if(vkCreateImageView(device, &view_info, VK_NULL_HANDLE, &p_vulkan_swapchain->p_image_views[i]) != VK_SUCCESS)
            return error_init(ERR_SRC_VULKAN, VULKAN_ERR_IMAGE_VIEW, "Failed to create image view");
    }

    return SUCCESS;
}

void vulkan_swapchain_destroy(void* p_void_vulkan_swapchain_del_struct) {
    LOG_DEBUG("Callback: %s", __func__);

    if(p_void_vulkan_swapchain_del_struct == NULL) {
        LOG_ERROR("%s: p_void_vulkan_swapchain_del_struct is NULL", __func__);
        return;
    }

    // Cast pointer
    vulkan_swapchain_del_struct_t* p_vulkan_swapchain_del_struct =
        (vulkan_swapchain_del_struct_t*)p_void_vulkan_swapchain_del_struct;

    if(p_vulkan_swapchain_del_struct->device == NULL) {
        LOG_ERROR("%s: device is NULL", __func__);
        return;
    }

    if(p_vulkan_swapchain_del_struct->vulkan_swapchain.swapchain == NULL) {
        LOG_ERROR("%s: swapchain is NULL", __func__);
        return;
    }

    // Destroy swapchain
    vkDestroySwapchainKHR(
        p_vulkan_swapchain_del_struct->device, p_vulkan_swapchain_del_struct->vulkan_swapchain.swapchain, VK_NULL_HANDLE
    );

    free(p_vulkan_swapchain_del_struct // NOLINT(bugprone-multi-level-implicit-pointer-conversion)
             ->vulkan_swapchain.p_images);
    p_vulkan_swapchain_del_struct->vulkan_swapchain.p_images = NULL;

    for(uint32_t i = 0; i < p_vulkan_swapchain_del_struct->vulkan_swapchain.images_count; ++i) {
        LOG_DEBUG("    Destroying swapchain image view, index: %u", i);
        vkDestroyImageView(
            p_vulkan_swapchain_del_struct->device, p_vulkan_swapchain_del_struct->vulkan_swapchain.p_image_views[i],
            VK_NULL_HANDLE
        );
    }

    free(p_vulkan_swapchain_del_struct // NOLINT(bugprone-multi-level-implicit-pointer-conversion)
             ->vulkan_swapchain.p_image_views);
    p_vulkan_swapchain_del_struct->vulkan_swapchain.p_image_views = NULL;

    free(p_vulkan_swapchain_del_struct);
    p_vulkan_swapchain_del_struct = NULL;
    p_void_vulkan_swapchain_del_struct = NULL;
}

static VkSurfaceFormatKHR choose_swapchain_surface_format(VkSurfaceFormatKHR* p_formats, size_t formats_count) {
    // For the color space we’ll use sRGB, which is pretty much the standard color space for viewing and printing
    // purposes, like the textures we’ll use later on. Because of that we should also use an sRGB color format, of
    // which one of the most common ones is VK_FORMAT_B8G8R8A8_SRGB.

    // Why not VK_FORMAT_B8G8R8A8_SRGB?
    // https://www.khronos.org/opengl/wiki/Image_Format
    // https://registry.khronos.org/vulkan/specs/latest/man/html/VkFormat.html

    for(size_t i = 0; i < formats_count; ++i) {
        if(p_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && // Used in the cppvk13 tutorial
           p_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return p_formats[i];
        }
    }
    // If that also fails then we could start ranking the available formats based on how "good" they are, but in
    // most cases it’s okay to just settle with the first format that is specified.
    return p_formats[0];
}

static VkPresentModeKHR choose_swapchain_present_mode(VkPresentModeKHR* p_present_modes, size_t present_modes_count) {
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

static VkExtent2D choose_swapchain_extent(SDL_Window* p_window, VkSurfaceCapabilitiesKHR capabilities) {
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
        int width = 0, height = 0;
        // SDL_GetWindowSize(p_engine->p_SDL_window, &width, &height);
        if(!SDL_GetWindowSizeInPixels(p_window, &width, &height)) {
            LOG_WARN("Failed to get the window size: %s", SDL_GetError());
            width = 0;
            height = 0;
        }

        VkExtent2D actual_extent = {(uint32_t)width, (uint32_t)height};

        // We "clamp" the width and height so that they are within the bounds of the allowed min/max values supported by
        // the implementation
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
