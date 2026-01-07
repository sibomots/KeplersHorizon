//////////////////////////////////////////////////////////////////////////////////
// This file is part of Kepler's Horizon
//
// Licensed under BSD 3-Clause License
//
// Copyright (c) 2025, sibomots
/////////////////////////////////////////////////////////////////////////////////
#ifndef __MODULEUTIL_H__
#define __MODULEUTIL_H__

/**
 * Get the module_id for a game from the games table.
 * Returns 1 (default Kepler's Horizon) if game not found.
 * 
 * @param game_id The game ID to lookup
 * @return The module_id associated with the game
 */
int get_module_id_for_game(int game_id);

#endif
