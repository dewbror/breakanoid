/*
  game.h
*/

#ifndef GAME_H_
#define GAME_H_
#pragma once

#include "types.h"

/**
 * A struct containing all the necessary game fields. Name may change from "game" to something else.
 */
typedef struct game_s {
    int temp;
    struct deletion_queue *p_delq;
} game_s;

bool game_run(struct vulkan_engine *p_engine, game_s *p_game);
bool game_destroy(game_s *p_game);
#endif // GAME_H_
