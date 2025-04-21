#ifndef VULKAN_ENGINE_H_
#define VULKAN_ENGINE_H_
#pragma once

/**
 * vk_engine.h
 */

#include "vulkan/vulkan_types.h"
// I dont want to include deletion queue here but temporarilty doing so because i cant get the forward decleration to work.
#include "util/deletion_queue.h"

typedef struct {
    VkImage *data;
    uint32_t sz;
} swapchain_images;

typedef struct {
    // Forward-decleration of deletion_queue not working :(
    // struct deletion_queue *p_del_queue;
    deletion_queue *p_main_delq; // Allocated on heap, move to stack?
    struct SDL_Window *p_SDL_window;
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
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

} vulkan_engine;

bool vulkan_engine_init(vulkan_engine *p_engine);
void vulkan_engine_destroy(vulkan_engine *p_engine);
#endif // VULKAN_ENGINE_H_
