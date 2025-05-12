/*
  vulkan_engine.h
*/

#ifndef VULKAN_ENGINE_H_
#define VULKAN_ENGINE_H_
#pragma once

#include <stdbool.h>

#include <vulkan/vulkan_core.h>

// Vulkan headers

/**
 * A struct containing a pointer to an array of VkImages and the size of the array in number of VkImages.
 */
typedef struct swapchain_images {
    VkImage* p_images;
    VkImageView* p_image_views;
    uint32_t images_count;
} swapchain_images;

/**
 * A struct containing all the necessary vulkan fields.
 */
typedef struct vulkan_engine {
    struct deletion_queue* p_main_delq;
    struct SDL_Window* p_SDL_window;
    VkExtent2D window_extent;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;
    VkSampleCountFlagBits msaa_samples;
    VkSwapchainKHR swapchain;
    swapchain_images swapchain_images;
    VkFormat swapchain_image_format;
    VkExtent2D swapchain_extent;
} vulkan_engine;

/**
 * Initiate the vulkan engine.
 *
 * \param[in] p_engine Pointer to the vulkan_engine to be initiated. Must be deleted using vulkan_engine_destroy before
 * exiting game.
 *
 * \return True if successful, false if failed.
 */
bool vulkan_engine_init(vulkan_engine* p_engine);

/**
 * Used to delete a vulkan_engine. All deletion/destruction of objects is currently handled by the deletion queue. All
 * this function does currently is flush the deletion queue.
 *
 * \param[in] p_engine Pointer to the vulkan_engine.
 *
 * \return True if successful, false otherwise
 */
bool vulkan_engine_destroy(vulkan_engine* p_engine);
#endif // VULKAN_ENGINE_H_
