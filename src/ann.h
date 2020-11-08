/**
* @file ann.h
* Header file for the core ann functions
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
* Made by Cansian

*/
#include "floatfann.h"

void ann_move_train(struct char_data *ch,	room_rnum was_in, room_rnum going_to);