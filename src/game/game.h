#ifndef GAME_H_
#define GAME_H_

#include "error/error.h"
/**
 * A struct containing all the necessary game fields. Name may change from "game" to something else.
 */
typedef struct game_s {
    int temp;
    struct deletion_stack_s* p_del_stack;
} game_t;

/**
 * \brief Initiate the game.
 */
error_t game_init(struct vulkan_engine_s* p_engine, game_t* p_game);

/**
 * \brief Destroy the game.
 */
error_t game_destroy(game_t* p_game);

/**
 * \brief Run game.
 */
error_t game_run(struct vulkan_engine_s* p_engine, game_t* p_game);

#endif // GAME_H_
