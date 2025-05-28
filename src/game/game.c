#include <stdbool.h>

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
