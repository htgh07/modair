#ifndef RELAY_H
#define RELAY_H

#include "common.h"

#define RELAY1t                   TRISAbits.TRISA3
#define RELAY1                    LATAbits.LATA3

void relay_init(void);
void relay_toggle(void);

#endif
