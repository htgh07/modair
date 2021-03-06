#include "xc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "config_bits.h"
#include "led.h"
#include "buzzer.h"
#include "rot_enc.h"
#include "glcd.h"
#include "glcd_lib.h"
#include "ecan_mod.h"
#include "widgets.h"
#include "menu_functions.h"
#include "modair_bus.h"
#include "heap.h"

//==============================================================================
//--------------------GLOBAL VARIABLES------------------------------------------
//==============================================================================
volatile __attribute__((far)) u16 heap_mem[HEAP_MEM_SIZE];
volatile u8 heap_item_cnt = 0;
volatile u8 heap_alloc = 0;

volatile s_pid_val pid_vals[64];
volatile u16 pid_vals_cnt = 0;

volatile u8 rot_enc_input = 0;

// use function pointers to navigate through the menu;
// default screen after bootup: home screen
void* (*current_menu_fnc)(u8) = &menu_fnc_homescreen;


//==============================================================================
//--------------------INTERRUPTS------------------------------------------------
//==============================================================================
void __attribute__((interrupt, auto_psv)) _T1Interrupt(void)
{
    rot_enc_tmr();
    _T1IF = 0;  // clear the interrupt
}

void __attribute__((interrupt, auto_psv)) _CNInterrupt(void)
{
    rot_enc_irq();
    _CNIF = 0;  // clear the interrupt
}

void __attribute__((interrupt, auto_psv)) _C1Interrupt(void)
{
    ecan_irq();
    _C1IF = 0;  // clear the interrupt
}

//==============================================================================
//--------------------INIT FUNCTIONS--------------------------------------------
//==============================================================================
void osc_init(void)
{
    OSCTUN = 0; // 7.37 MHz
    CLKDIV = 0; // N1=2, N2=2
    PLLFBD = 63; // M=65
    // Fosc = 7.37*M/(N1*N2) = 119.7625 MHz
    // Fcy  = Fosc/2 = 59.88125 MIPS
    while (OSCCONbits.LOCK!=1){}; // Wait for PLL to lock
}

void tmr1_init(u16 freq_hz)
{
    T1CON = 0b1000000000110000; // TMR1 on, 1:256 prescale, Fosc/2
    PR1 = F_CY/256/freq_hz;
}

void irq_init(void)
{
    _T1IF = 0; // Timer1 Flag
    _T1IP = 2; // second lowest priority level
    _T1IE = ENABLE; // timer1 interrupt enable

    _CNIF = 0; // ChangeNotification Flag
    _CNIP = 1; // lowest priority level
    _CNIE = ENABLE; // change notification interrupt enable

    _C1IF = 0; // CAN1 Event Interrupt Flag
    _C1IP = 3; // third lowest priority
    _C1IE = ENABLE; // CAN1 Event Interrupt Enable
}

//==============================================================================
//--------------------COMMUNICATIONS HANDLER------------------------------------
//==============================================================================
void ecan_rx(u16 pid, u16 *data, u8 msg_type, u8 flags, u8 len)
{
    if ((heap_alloc==HEAP_ALLOC_CANDEBUG)&&(heap_item_cnt<8)) { // Debug-Bus menu is active
        s_can_debug* can_dbgs = (s_can_debug*)&heap_mem[0];
        can_dbgs[heap_item_cnt].pid = pid;
        can_dbgs[heap_item_cnt].d0 = data[0];
        can_dbgs[heap_item_cnt].d2 = data[1];
        can_dbgs[heap_item_cnt].d4 = data[2];
        can_dbgs[heap_item_cnt].d6 = data[3];
        can_dbgs[heap_item_cnt].msg_type = msg_type;
        can_dbgs[heap_item_cnt].len = len;
        can_dbgs[heap_item_cnt].flags = flags;
        heap_item_cnt++;
    }

    switch (msg_type) {
        case MT_BROADCAST_VALUE: {
                float *val = (float*)data;
                u16 i;
                for (i=0;i<pid_vals_cnt;i++)
                if (pid_vals[i].pid == pid) {
                    pid_vals[i].val = *val;
                    break;
                }
            }
            break;
        case MT_BROADCAST_NAME:
            if ((heap_alloc==HEAP_ALLOC_PIDNAME)&&(heap_item_cnt<MAX_PID_NAME_ITEMS)&&(flags==FT_PKT_SINGLE)) {
                s_pid_name* pid_names = (s_pid_name*)&heap_mem[0];
                pid_names[heap_item_cnt].pid = pid;
                pid_names[heap_item_cnt].u.nval[0] = data[0];
                pid_names[heap_item_cnt].u.nval[1] = data[1];
                pid_names[heap_item_cnt].u.nval[2] = data[2];
                pid_names[heap_item_cnt].u.nval[3] = data[3];
                if (len<8) // terminate string
                    pid_names[heap_item_cnt].u.name[len] = 0x00;
                heap_item_cnt++;
            }
            break;
        case MT_CONSOLE_TEXT:
            if (heap_alloc==HEAP_ALLOC_CONSOLETXT) {
                s_console_txt* console_txt = (s_console_txt*)&heap_mem[0];
                if (console_txt->pid == pid) {
                    if ((flags==FT_PKT_START)||(flags==FT_PKT_SINGLE)) {
                        memset((char*)console_txt->txt, ' ', 16*4);
                        heap_item_cnt=0;
                    }
                    if (heap_item_cnt<8) {
                        memcpy((char*)&console_txt->txt[heap_item_cnt*8], (char*)&data[0], len);
                        heap_item_cnt++;
                    }
                }
            }
            break;
    }
}



//==============================================================================
//--------------------MAIN LOOP-------------------------------------------------
//==============================================================================
int main(void)
{
    osc_init();
    led_init();
    buzzer_init();
    rot_enc_init();
    lcd_init();
    ecan_init();
    tmr1_init(50); // 50 Hz == 20 ms ticks
    irq_init();

    read_widgets();

    while(1) {
        // clear lcd-buffer
        lcd_clrbuff();

        // latch input from rotary-encoder (changed during IRQs)
        u8 rot_enc_input_b = rot_enc_input;
        rot_enc_input = 0;

        // extra-long-press always gets you back to home-screen
        if ((rot_enc_input_b == C_ROT_EXTRALONGHOLD) || (current_menu_fnc == NULL)) {
            current_menu_fnc = &menu_fnc_homescreen;
            heap_alloc = HEAP_ALLOC_NONE; // deallocate heap
            rot_enc_input_b = 0;
        }

        // call current menu-function and update next menu-function pointer
        current_menu_fnc = (*current_menu_fnc)(rot_enc_input_b);

        // copy lcd-buffer to lcd
        lcd_update();
        led_toggle();
    }
}
//==============================================================================
//==============================================================================
