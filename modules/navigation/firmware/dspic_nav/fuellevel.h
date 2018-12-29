#ifndef FUELLEVEL_H
#define FUELLEVEL_H

#include "common.h"

typedef struct {
    u16 FLx[16];
    s16 FLy[16];
} s_fuelcal;

void fuellevel_cntdwn(u8 idx);

void* fuellevel_fnc_homescreen(u8 idx, u8 key_input);

#endif
