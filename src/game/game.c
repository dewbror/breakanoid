#include <stdbool.h>

#include "logger.h"
#include "vulkan/vulkan_engine.h"
#include "game/game.h"
#include "util/deletion_queue.h"

/**
 *
 */
bool game_run(struct vulkan_engine_s* p_engine, game_t* p_game) {
    // UNUSED
    (void)p_engine;
    (void)p_game;

    // Allocate game deletion queue
    p_game->p_delq = deletion_queue_alloc();
    if(p_game->p_delq == NULL) {
        // Handle deletion_queue_alloc error
        LOG_ERROR("Failed to allocate deletion queue");
        return false;
    }
    LOG_INFO("Game running");
    return true;
}

/**
 *
 */
bool game_destroy(game_t* p_game) {
    if(p_game == NULL) {
        LOG_ERROR("game_destroy: p_game is NULL");
        return false;
    }
    // Flush deletion queue
    if(!deletion_queue_flush(&p_game->p_delq)) {
        LOG_ERROR("Failed flush deletion queue");
        return false;
    }

    // Check that p_delq is NULL
    if(p_game->p_delq != NULL) {
        LOG_ERROR("Failed to flush deletion queue");
        return false;
    }

    LOG_INFO("Game destroyed");
    return true;
}
