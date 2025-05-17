/*
  game.h
*/

#ifndef GAME_H_
#define GAME_H_
#pragma once

#include <stdbool.h>

/**
 * A struct containing all the necessary game fields. Name may change from "game" to something else.
 */
typedef struct game_s {
    int temp;
    struct deletion_queue_s* p_delq;
} game_t;

bool game_run(struct vulkan_engine_s* p_engine, game_t* p_game);
bool game_destroy(game_t* p_game);
#endif // GAME_H_
