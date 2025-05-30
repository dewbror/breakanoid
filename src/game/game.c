#include <stdbool.h>

#include <SDL3/SDL.h>

#include "error/error.h"

#include "logger.h"
#include "vulkan/vulkan_engine.h"
#include "game/game.h"
#include "util/deletion_stack.h"

error_t game_init(struct vulkan_engine_s* p_engine, game_t* p_game) {
    // UNUSED
    (void)p_engine;
    (void)p_game;

    // Allocate game deletion queue
    p_game->p_del_stack = deletion_stack_init();
    if(p_game->p_del_stack == NULL)
        return error_init(ERR_SRC_CORE, ERR_DELETION_STACK_INIT, "%s: Failed to initiate queue stack", __func__);

    LOG_INFO("Game initialized");

    return SUCCESS;
}

error_t game_destroy(game_t* p_game) {
    if(p_game == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: p_game is NULL", __func__);

    // Flush deletion queue
    error_t err = deletion_stack_flush(&p_game->p_del_stack);
    if(err.code != 0)
        return err;

    LOG_INFO("Game destroyed");

    return SUCCESS;
}

error_t game_run(struct vulkan_engine_s* p_engine, game_t* p_game) {
    // UNUSED
    (void)p_game;

    SDL_Event e;
    bool quit = false;
    bool stop_rendering = false;

    while(!quit) {
        while(SDL_PollEvent(&e) != 0) {
            switch(e.type) {
                case SDL_EVENT_QUIT:
                    quit = true;
                    break;
                case SDL_EVENT_WINDOW_MINIMIZED:
                    LOG_DEBUG("Window minimized");
                    stop_rendering = true;
                    break;
                case SDL_EVENT_WINDOW_RESTORED:
                    LOG_DEBUG("Windows restored");
                    stop_rendering = false;
                    break;
                default:
                    break;
            }
            // Send SDL event to imgui here
            // ImGui_ImplSDL3_ProcessEvent(&e);
        }

        if(stop_rendering) {
            SDL_Delay(100);
            continue;
        }

        // Draw imgui here

        // Perform drawing here
        vulkan_engine_render_and_present_frame(p_engine);
    }

    return SUCCESS;
}
