#ifndef PARAMS_H
#define PARAMS_H

#include "common.h"

// +1 for module       (sizeof(PARAM_LIST)/sizeof(s_param_settings))
#define PARAM_CNT      14

typedef struct {
    u16 pid;
    char name[8];
    u16 rate;
} s_param_settings;

typedef struct {
    void (*canrx_fnc_ptr)(u8,u16,u16*,u8,u8,u8);
    void (*sendval_fnc_ptr)(u8);
    void* (*menu_fnc_ptr)(u8,u8);
} s_param_const;


#endif
