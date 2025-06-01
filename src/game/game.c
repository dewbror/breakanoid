#include <stdbool.h>

#include <SDL3/SDL.h>

#include "error/error.h"
#include "logger.h"

#include "vulkan/vulkan_context.h"
#include "game/game.h"
#include "util/deletion_stack.h"

error_t game_init(struct vulkan_context_s* p_vkctx, game_t* p_game)
{
    // UNUSED
    (void)p_vkctx;
    (void)p_game;

    // Allocate game deletion queue
    p_game->p_dstack = deletion_stack_init();
    if(p_game->p_dstack == NULL)
        return error_init(ERR_SRC_CORE, ERR_DELETION_STACK_INIT, "%s: Failed to initiate queue stack", __func__);

    LOG_INFO("Game initialized");

    return SUCCESS;
}

error_t game_deinit(game_t* p_game)
{
    if(p_game == NULL)
        return error_init(ERR_SRC_CORE, ERR_NULL_ARG, "%s: p_game is NULL", __func__);

    // Flush deletion queue
    error_t err = deletion_stack_flush(&p_game->p_dstack);
    if(err.code != 0)
        return err;

    LOG_DEBUG("Game deinitialized");

    return SUCCESS;
}

error_t game_run(struct vulkan_context_s* p_vkctx, game_t* p_game)
{
    // UNUSED
    (void)p_game;

    SDL_Event e;
    bool quit = false;
    bool stop_rendering = false;

    while(!quit) {
        while(SDL_PollEvent(&e) != 0) {
            switch(e.type) {
            case SDL_EVENT_QUIT:
                LOG_INFO("Quiting game")
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
        vulkan_render_and_present_frame(p_vkctx);
    }

    return SUCCESS;
}
