#ifndef COMMON_H
#define COMMON_H

#include "xc.h"

typedef signed char s8;
typedef signed int s16;
typedef signed long int s32;
typedef signed long long int s64;
typedef unsigned char u8;
typedef unsigned int u16;
typedef unsigned long int u32;
typedef unsigned long long int u64;

#define F_CY                    59881250

#define INPUT_PIN               1
#define OUTPUT_PIN              0
#define ANALOG_PIN              1
#define DIGITAL_PIN             0
#define ENABLE                  1
#define DISABLE                 0

void __delay32(u32 cycles);
#define delay_ns(ns)            __delay32((u32)ns / 62.5)
#define delay_us(us)            __delay32((u32)us * 16)
#define delay_ms(ms)            __delay32((u32)ms * 16000)

int mpow(int x, int exp);
char *mprint_int(char *s, int n, int base, int digits);
char *mprint_float(char *str, float val, int pre_digits, int post_digits);

#endif
