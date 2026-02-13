///////////////////////////////////////////
// This file is part of Kepler's Horizon //
//                                       //
// Licensed under BSD 3-Clause License   //
//                                       //
// Copyright (c) 2025, sibomots          //
///////////////////////////////////////////
#ifndef __KH_INIT_H__
#define __KH_INIT_H__

void pre_init(void);
void init(void);
void banner(void);
void apply_arguments(int, char**);
void load_services();
void run(void);
#endif
