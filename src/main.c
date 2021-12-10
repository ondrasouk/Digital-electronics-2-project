/***********************************************************************
 * 
 * Analog-to-digital conversion with displaying result on LCD and 
 * transmitting via UART.
 * ATmega328P (Arduino Uno), 16 MHz, AVR 8-bit Toolchain 3.6.2
 *
 * Copyright (c) 2018-Present Tomas Fryza
 * Dept. of Radio Electronics, Brno University of Technology, Czechia
 * This work is licensed under the terms of the MIT license.
 * 
 **********************************************************************/

/* Includes ----------------------------------------------------------*/
#include <avr/io.h>         // AVR device-specific IO definitions
#include <avr/interrupt.h>  // Interrupts standard C library for AVR-GCC
#include <util/atomic.h>    // Atomic operations
#include "timer.h"          // Timer library for AVR-GCC
#include "lcd.h"            // Peter Fleury's LCD library
#include <stdlib.h>         // C library. Needed for conversion function
#include <util/delay.h>
#include "uart.h"           // Peter Fleury's UART library
#include "lcd_buttons.h"    // Library with reading buttons
#include <string.h>


#define DISPLAY_ROWS 2
#define MENU_TEXT_SIZE_D 10
#define MENU_TEXT_SHIFT_TIME_USEC 150000
#define MENU_ITEM_SIZE_D 5
#define TIM2_OVERFLOW_TIME 4096
/* Globals -----------------------------------------------------------*/
// Custom character definition using https://omerk.github.io/lcdchargen/
const uint8_t customChar[] = {
    0b11000, 0b11100, 0b10110, 0b10011, 0b10011, 0b10110, 0b11100,
    0b11000, // 0
    0b10000, 0b11000, 0b11100, 0b11110, 0b11110, 0b11100, 0b11000,
    0b10000, // 1
    0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000,
    0b10000, // 2
    0b11000, 0b11000, 0b11000, 0b11000, 0b11000, 0b11000, 0b11000,
    0b11000, // 3
    0b11100, 0b11100, 0b11100, 0b11100, 0b11100, 0b11100, 0b11100,
    0b11100, // 4
    0b11110, 0b11110, 0b11110, 0b11110, 0b11110, 0b11110, 0b11110,
    0b11110, // 5
    0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111,
    0b11111, // 6
};
#define MENU_TEXT_SHIFT_NUM MENU_TEXT_SHIFT_TIME_USEC / TIM2_OVERFLOW_TIME
#if !defined(ARRAY_SIZE)
    #define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))
#endif
#define CURSOR_POS (menu_pos + scroll_pos)

// static uint8_t MENU_TEXT_SIZE = MENU_TEXT_SIZE_D;

typedef struct {
    const uint8_t type; // 1 - plain; 2 - with "%"; 3 - with "m"
    const uint8_t limit_min;
    const uint8_t step;
    const uint8_t limit_max;
    uint8_t *value;
} item_uint8;

const char *smart_menu[] = {
    "Irrigation mode:",
    "Days away:",
    "Irrigation level:",
    "Save water from water level:",
};

uint8_t days_away = 3;
uint8_t irrigation_level = 50;
uint8_t save_water_level = 20;

const item_uint8 smart_menu_items[] = {
    {1, 1, 1, 14, &days_away},
    {2, 0, 10, 100, &irrigation_level},
    {2, 0, 20, 100, &save_water_level},
};

const char *drip_menu[] = {
    "Irrigation mode:",
    "Max irrigation level:",
    "Min irritation level:",
    "Irritation (mins):",
};

uint8_t irrigation_max = 80;
uint8_t irrigation_min = 30;
uint8_t irrigation_duration_min = 20;

const item_uint8 drip_menu_items[] = {
    {2, 10, 10, 100, &irrigation_max},
    {2, 0, 10, 100, &irrigation_min},
    {3, 0, 1, 255, &irrigation_duration_min},
};

const char *const_menu[] = {
    "Irrigation mode:",
    "Irritation level:",
    "Hysterezis:",
    "Irritation (mins):",
    "Wait (min):",
};

uint8_t irrigation_hysterezis = 20;
uint8_t irrigation_wait_min = 100;

const item_uint8 const_menu_items[] = {
    {2, 1, 1, 14, &irrigation_level},
    {2, 10, 10, 100, &irrigation_hysterezis},
    {3, 0, 1, 255, &irrigation_duration_min},
    {3, 0, 1, 255, &irrigation_wait_min},
};

const char *menu_head_items[] = {
    "smart",
    " drip",
    "const",
};

const char **mode_menu_pointers[] = {
    smart_menu,
    drip_menu,
    const_menu,
}; // Add all menus to display

const item_uint8 *mode_menu_items[] = {
    smart_menu_items,
    drip_menu_items,
    const_menu_items,
};

const uint8_t mode_menu_lenght[] = {
    ARRAY_SIZE(smart_menu),
    ARRAY_SIZE(drip_menu),
    ARRAY_SIZE(const_menu),
};

const char type_suffix[4] = "  %m";

uint16_t ADC_key_value = 65535;
uint8_t menu_pos = 0;
uint8_t scroll_pos = 0;

uint8_t slice_start = 0;
const char *shift_string;
char shift_buffer[MENU_TEXT_SIZE_D + 1] = "         \x7e"; // last character is left arrow + end of string
uint8_t scroll_pos_shift = 255;

char value_disp[MENU_ITEM_SIZE_D + 1] = "     ";

uint8_t TIM2_flag = 0;

uint8_t irrigation_mode = 0;

/* Function definitions ----------------------------------------------*/

void slice_str(const char *str, char *buffer, uint8_t start)
{
    strncpy(buffer, str + start, MENU_TEXT_SIZE_D - 1);
}

void itoa_menu_item(int value, char *str, uint8_t type)
{
    strcpy(str, "     "); // Clear output buffer
    if(value < 10){
        itoa(value, str + MENU_ITEM_SIZE_D - 2, 10);
    }
    else if(value < 100){
        itoa(value, str + MENU_ITEM_SIZE_D - 3, 10);
    }
    else if(value < 1000){
        itoa(value, str + MENU_ITEM_SIZE_D - 4, 10);
    }
    else if(value < 10000){
        itoa(value, str + MENU_ITEM_SIZE_D - 5, 10);
    }
    else{
        itoa(value, str, 10);
    }
    str[4] = type_suffix[type];
}

void TIM2_routine() 
{
    if(scroll_pos_shift != 255){
        if (TIM2_flag > MENU_TEXT_SHIFT_NUM) {
            TIM2_flag = 0;
            if (slice_start > (strlen(shift_string) - MENU_TEXT_SIZE_D)){
                slice_start = 0;
                shift_buffer[MENU_TEXT_SIZE_D - 1] = 0x7e;
            }
            else if(slice_start == (strlen(shift_string) - MENU_TEXT_SIZE_D)){
                strncpy(shift_buffer, shift_string + slice_start, MENU_TEXT_SIZE_D);
            }
            slice_str(shift_string, shift_buffer, slice_start);
            lcd_gotoxy(1, scroll_pos_shift);
            lcd_puts(shift_buffer);
            
            slice_start++;
        }
    }
    else{
        TIM2_flag = 0;
    }
}

void menu_print(const char *menu_entries[])
{
    lcd_clrscr();
    lcd_gotoxy(1,0);
    lcd_gotoxy(1, 0); // Fix some unknown problem
    scroll_pos_shift = 255;
    if(strlen(menu_entries[menu_pos]) > MENU_TEXT_SIZE_D){
        slice_start = 0;
        slice_str(menu_entries[menu_pos], shift_buffer, 0);
        lcd_puts(shift_buffer);
        if(scroll_pos == 0){
            shift_string = menu_entries[menu_pos];
            scroll_pos_shift = 0;
        }
    }
    else{
        lcd_puts(menu_entries[menu_pos]);
    }
    lcd_gotoxy(1,1);
    if(strlen(menu_entries[menu_pos+1]) > MENU_TEXT_SIZE_D){
        slice_start = 0;
        slice_str(menu_entries[menu_pos+1], shift_buffer, 0);
        lcd_puts(shift_buffer);
        if(scroll_pos == 1){
            shift_string = menu_entries[menu_pos+1];
            scroll_pos_shift = 1;
        }
    }
    else{
        lcd_puts(menu_entries[menu_pos+1]);
    }
    lcd_gotoxy(0,scroll_pos);
    lcd_putc(0); //Print cursor >
    if(menu_pos == 0){
        lcd_gotoxy(11, 0);
        lcd_puts(menu_head_items[irrigation_mode]);
        lcd_gotoxy(11, 1);
        itoa_menu_item(*mode_menu_items[irrigation_mode][menu_pos].value, value_disp, mode_menu_items[irrigation_mode][menu_pos].type);
        lcd_puts(value_disp);
    }
    else{
        lcd_gotoxy(11, 0);
        itoa_menu_item(*mode_menu_items[irrigation_mode][menu_pos-1].value, value_disp, mode_menu_items[irrigation_mode][menu_pos-1].type);
        lcd_puts(value_disp);
        lcd_gotoxy(11, 1);
        itoa_menu_item(*mode_menu_items[irrigation_mode][menu_pos].value, value_disp, mode_menu_items[irrigation_mode][menu_pos].type);
        lcd_puts(value_disp);
    }
    
}

void menu(uint8_t key_press)
{
    if(key_press == 3){
        scroll_pos++;
        if((menu_pos < mode_menu_lenght[irrigation_mode]-2) & (scroll_pos == 2)){
            menu_pos++;
            scroll_pos--;
        }
        else if(scroll_pos == 2){
            scroll_pos--;
        }
    }
    if(key_press == 4){
        scroll_pos--;
        if((menu_pos > 0) & (scroll_pos == 255)){
            menu_pos--;
            scroll_pos++;
        } 
        else if (scroll_pos == 255){
            scroll_pos++;
        }
    }
    item_uint8 menu_item = mode_menu_items[irrigation_mode][CURSOR_POS - 1];
    uint8_t item_value;
    if(key_press == 5){
        if(CURSOR_POS == 0){
            irrigation_mode++;
            if(irrigation_mode > ARRAY_SIZE(mode_menu_lenght) - 1){
                irrigation_mode = 0;
            }
        }
        else{
            item_value = *menu_item.value;
            *menu_item.value += menu_item.step;
            if((*menu_item.value > menu_item.limit_max)|((*menu_item.value - item_value) != menu_item.step)){
                *menu_item.value = menu_item.limit_max;
            }
        }
    }
    if(key_press == 2){
        if(CURSOR_POS == 0){
            irrigation_mode--;
            if(irrigation_mode == 255){
                irrigation_mode = ARRAY_SIZE(mode_menu_lenght) - 1;
            }
        }
        else{
            item_value = *menu_item.value;
            *menu_item.value -= menu_item.step;
            if((*menu_item.value < menu_item.limit_min)|((item_value - *menu_item.value) != menu_item.step)){
                *menu_item.value = menu_item.limit_min;
            }
        }
    }
    menu_print(mode_menu_pointers[irrigation_mode]);
    shift_buffer[MENU_TEXT_SIZE_D - 1] = 0x7e;
}


/**********************************************************************
 * Function: Main function where the program execution begins
 * Purpose:  TODO
 * Returns:  none
 **********************************************************************/
int main(void)
{
    // Initialize LCD display
    lcd_init(LCD_DISP_ON);
    // Upload custom characters
    lcd_command(1 << LCD_CGRAM);
    for (uint8_t i = 0; i < sizeof(customChar); i++)
    {
        lcd_data(customChar[i]);
    }
    lcd_command(1 << LCD_DDRAM);
    // Print debug info about the menu
    lcd_gotoxy(0, 0);
    char str[16] = "                ";
    itoa(mode_menu_lenght[1], str, 10);
    lcd_puts(str);
    lcd_puts(" ");
    lcd_puts(mode_menu_pointers[0][1]);
    // Print the custom characters on display
    lcd_gotoxy(0, 1);
    for (uint8_t i = 0; i < sizeof(customChar)/8; i++)
    {
        lcd_putc(i);
    }    
    _delay_ms(1000);
    // Configure ADC to convert PC0[A0] analog value
    // Set ADC reference to AVcc
    ADMUX |= (1 << REFS0);
    // Set input channel to ADC0
    ADMUX &= ~((1 << MUX0) | (1 << MUX1) | (1 << MUX2) | (1 << MUX3));
    // Enable ADC module
    ADCSRA |= (1 << ADEN);
    // Enable conversion complete interrupt
    ADCSRA |= (1 << ADIE);
    // Set clock prescaler to 128
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    // Configure 16-bit Timer/Counter2 to start ADC conversion
    // Set prescaler to 16 ms and enable overflow interrupt
    TIM2_overflow_16ms();
    TIM2_overflow_interrupt_enable();
    
    
    // Initialize UART to asynchronous, 8N1, 9600
    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    // Enables interrupts by setting the global interrupt mask
    sei();
    
    uint8_t key_press = 0;
    menu(0);
    // Infinite loop
    while (1) {
        TIM2_routine();
        key_press = key_press_detect(&ADC_key_value);
        if (key_press) {
            char str[1] = "0";
            itoa(key_press, str, 10);
            uart_puts("\n\rKey pressed: ");
            uart_puts(str);
            menu(key_press);
        }
        
    }

    // Will never reach this
    return 0;
}

/* Interrupt service routines ----------------------------------------*/
/**********************************************************************
 * Function: Timer/Counter2 overflow interrupt
 * Purpose:  Use single conversion mode and start.
 **********************************************************************/
ISR(TIMER2_OVF_vect)
{
    // Start ADC conversion
    TIM2_flag++;
    ADCSRA |= (1 << ADSC);
}

/**********************************************************************
 * Function: ADC complete interrupt
 * Purpose:  Copy ADC value.
 **********************************************************************/
ISR(ADC_vect) 
{
    ADC_key_value = ADC;  
}
