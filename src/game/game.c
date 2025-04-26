/*
  game.c
*/

#include "version.h"
#include "vulkan/vulkan_engine.h"
#include "util/deletion_queue.h"
#include "game.h"
#include "SDL/SDL_backend.h"

/**
 * 
 */
bool game_run(struct vulkan_engine *p_engine, game_s *p_game) {
    // UNUSED
    (void)p_engine;
    (void)p_game;

    // Allocate game deletion queue
    p_game->p_delq = deletion_queue_alloc();

    return true;
}

/**
 * 
 */
bool game_destroy(game_s *p_game) {
    // Flush deletion queue
    deletion_queue_flush(p_game->p_delq);
    return true;
}
