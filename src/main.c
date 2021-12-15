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
#include <stdlib.h>         // C library. Needed for conversion function
#include <util/delay.h>
#include <string.h>
#include "timer.h"          // Timer library for AVR-GCC
#include "lcd.h"            // Peter Fleury's LCD library
#include "uart.h"           // Peter Fleury's UART library
#include "lcd_buttons.h"    // Library with reading buttons
#include "hum_sens.h"
#include "level_sens.h"

/* Definitions -------------------------------------------------------*/
#define DISPLAY_ROWS 2
#define MENU_TEXT_SIZE_D 10
#define MENU_TEXT_SHIFT_TIME_USEC 150000
#define MENU_ITEM_SIZE_D 5
#define TIM2_OVERFLOW_TIME 4096
#define MENU_TIMEOUT_SECS 10

#if !defined(ARRAY_SIZE)
    #define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))
#endif
#define CURSOR_POS (menu_pos + scroll_pos)
#define MENU_TEXT_SHIFT_NUM MENU_TEXT_SHIFT_TIME_USEC / TIM2_OVERFLOW_TIME
#define MENU_TIMEOUT_NUM MENU_TIMEOUT_SECS * 100000 / TIM2_OVERFLOW_TIME

/*--------------------------------------------------------------------*/
/* Globals -----------------------------------------------------------*/
/*--------------------------------------------------------------------*/

/* Custom Characters -------------------------------------------------*/
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

/* Characters for type -----------------------------------------------*/
// "\xHH" for entering plain hexadecimal values (0-9 is user defined symbols)
const char type_suffix[5] = "  %ms";

/* Structures definitions --------------------------------------------*/
typedef struct {
    const uint8_t type; // 1 - plain; 2 - with "%"; 3 - with "m"; 4 - with "s"; higher is character menu items
    const uint8_t limit_min;
    const uint8_t step;
    const uint8_t limit_max;
    uint8_t *value;
} item_uint8;


/* Menu --------------------------------------------------------------*/

uint8_t irrigation_mode = 0; // Variable with what menu to display

const char *menu_head_items[] = {
    "smart",
    " drip",
    "const",
};

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
    {5, 0, 1, ARRAY_SIZE(menu_head_items)-1, &irrigation_mode},
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
    {5, 0, 1, ARRAY_SIZE(menu_head_items)-1, &irrigation_mode},
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
    {5, 0, 1, ARRAY_SIZE(menu_head_items)-1, &irrigation_mode},
    {2, 1, 1, 14, &irrigation_level},
    {2, 10, 10, 100, &irrigation_hysterezis},
    {3, 0, 1, 255, &irrigation_duration_min},
    {3, 0, 1, 255, &irrigation_wait_min},
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

const char **menu_character_values_pointers[] = {
    menu_head_items,
};

/* Display variables --------------------------------------------------*/

uint8_t menu_pos = 0;   // position of top item in menu (0 - x)
uint8_t scroll_pos = 0; // line of cursor 0 or 1

uint8_t slice_start = 0;
const char *shift_string;
char shift_buffer[MENU_TEXT_SIZE_D + 1] = "         \x7e"; // last characters is left arrow + end of string
uint8_t scroll_pos_shift = 255;

char value_disp[MENU_ITEM_SIZE_D + 1] = "     ";

uint8_t display_update = 1; // 0 - no update in menu, 1 - update in menu, 2 - update in menu values, 3 - no update in status page, 4 - update in status page, 5 - update in status page values

/* Global variables --------------------------------------------------*/

uint8_t TIM1_flag = 0;
uint8_t TIM2_flag = 0;
uint16_t TIM2_display_timer = 0;
uint16_t ADC_key_value = 65535;
uint16_t sens_humidity = 100;
uint8_t sens_water_level = 100;

/*--------------------------------------------------------------------*/
/* Function definitions ----------------------------------------------*/
/*--------------------------------------------------------------------*/

void cyclic_inc(uint8_t *x, const uint8_t lim_min, const uint8_t lim_max)
{
    uint8_t value = *x;
    *x = value + 1;
    if((*x > lim_max)|((*x - value) != 1)){
        *x = lim_min;
    }
}

void cyclic_dec(uint8_t *x, const uint8_t lim_min, const uint8_t lim_max)
{
    uint8_t value = *x;
    *x = value - 1;
    if((*x < lim_min)|((value - *x) != 1)){
        *x = lim_max;
    }
}

void limited_inc(uint8_t *x, const uint8_t lim, const uint8_t step)
{
    uint8_t value = *x;
    *x = value + step;
    if((*x > lim)|((*x - value) != step)){
        *x = lim;
    }
}

void limited_dec(uint8_t *x, const uint8_t lim, const uint8_t step)
{
    uint8_t value = *x;
    *x = value - step;
    if((*x < lim)|((value - *x) != step)){
        *x = lim;
    }
}

void slice_str(const char *str, char *buffer, uint8_t start)
{
    strncpy(buffer, str + start, MENU_TEXT_SIZE_D - 1);
}

void itoa_menu_item(int value, char *str, uint8_t type)
{
    strncpy(str, "     ", MENU_ITEM_SIZE_D); // Clear output buffer
    if(type < ARRAY_SIZE(type_suffix)){
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
    else{
        if((type - ARRAY_SIZE(type_suffix)) < ARRAY_SIZE(menu_character_values_pointers)){
            strcpy(str ,menu_character_values_pointers[type-ARRAY_SIZE(type_suffix)][value]);
        }
        else{
            str = "error";
        }
    }
}

void TIM1_routine() 
{
    if(TIM1_flag){
        char str[] = "     ";
        TIM1_flag = 0;
        sens_humidity = read_hum();
        sens_water_level = read_level();
        uart_puts("\n\rSoil Humidity: ");
        itoa_menu_item(sens_humidity, str, 2);
        uart_puts(str);
        uart_puts("\n\rWater level: ");
        itoa_menu_item(sens_water_level, str, 2);
        uart_puts(str);
        if(display_update == 3){
            display_update = 5;
        }
    }
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

void menu_line_print(const char *menu_entries[], uint8_t line)
{
    lcd_gotoxy(1,line);
    if(strlen(menu_entries[menu_pos + line]) > MENU_TEXT_SIZE_D){
        slice_start = 0;
        slice_str(menu_entries[menu_pos + line], shift_buffer, 0);
        lcd_puts(shift_buffer);
        if(scroll_pos == line){
            shift_string = menu_entries[menu_pos + line];
            scroll_pos_shift = line;
        }
    }
    else{
        lcd_puts(menu_entries[menu_pos + line]);
    }
    lcd_gotoxy(11, line);
    itoa_menu_item(*mode_menu_items[irrigation_mode][menu_pos+line].value, value_disp, mode_menu_items[irrigation_mode][menu_pos+line].type);
    lcd_puts(value_disp);
}

void display_menu(uint8_t key_press)
{
    display_update = 0;
    if(key_press == 3){ // UP Key
        scroll_pos++;
        if((menu_pos < mode_menu_lenght[irrigation_mode]-2) & (scroll_pos == 2)){ // scroll up
            menu_pos++;
            scroll_pos--;
            display_update = 1;
        }
        else if(scroll_pos == 2){ // top of menu
            scroll_pos--;
        }
    }
    if(key_press == 4){ // DOWN Key
        scroll_pos--;
        if((menu_pos > 0) & (scroll_pos == 255)){ // scroll down
            menu_pos--;
            scroll_pos++;
            display_update = 1;
        } 
        else if (scroll_pos == 255){ // bottom of menu
            scroll_pos++;
        }
    }
    item_uint8 menu_item = mode_menu_items[irrigation_mode][CURSOR_POS];
    if(key_press == 5){ // RIGHT Key
        if(menu_item.type >= ARRAY_SIZE(type_suffix)){
            cyclic_inc(menu_item.value, menu_item.limit_min, menu_item.limit_max);
        }
        else{
            limited_inc(menu_item.value, menu_item.limit_max, menu_item.step);
        }
        display_update = 2;
    }
    if(key_press == 2){  // LEFT Key
        if(menu_item.type >= ARRAY_SIZE(type_suffix)){
            cyclic_dec(&irrigation_mode, menu_item.limit_min, menu_item.limit_max);
        }
        else{
            limited_dec(menu_item.value, menu_item.limit_min, menu_item.step);
        }
        display_update = 2;
    }
    lcd_clrscr();
    lcd_gotoxy(1, 0); // Fix some unknown problem
    scroll_pos_shift = 255;
    menu_line_print(mode_menu_pointers[irrigation_mode], 0);
    menu_line_print(mode_menu_pointers[irrigation_mode], 1);
    lcd_gotoxy(0,scroll_pos);
    lcd_putc(0); // Print cursor >
    shift_buffer[MENU_TEXT_SIZE_D - 1] = 0x7e;
}

void display_status() 
{
    if((display_update < 3)|(display_update == 4)){
        char str[] = "Soil:           ";
        lcd_clrscr();
        lcd_gotoxy(0, 0); // Fix some unknown problem
        lcd_gotoxy(0, 0);
        itoa_menu_item(sens_humidity, str+6, 2);
        lcd_puts(str);
        strcpy(str, "Water:          ");
        lcd_gotoxy(0, 1);
        itoa_menu_item(sens_water_level, str+6, 2);
        lcd_puts(str);
        display_update = 3;
    } 
    else if (display_update == 5) {
        lcd_gotoxy(6, 0);
        itoa_menu_item(sens_humidity, value_disp, 2);
        lcd_puts(value_disp);
        lcd_gotoxy(6, 1);
        itoa_menu_item(sens_water_level, value_disp, 2);
        lcd_puts(value_disp);
        display_update = 3;
    }
    
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
    // Configure 8-bit Timer/Counter2 to start ADC conversion
    // Set prescaler to 16 ms and enable overflow interrupt
    TIM2_overflow_16ms();
    TIM2_overflow_interrupt_enable();
    // Configure 16-bit Timer/Counter1 to start sensors readout
    // Set prescaler to 4 s and enable overflow interrupt
    TIM1_overflow_1s();
    TIM1_overflow_interrupt_enable();
    // Initialize UART to asynchronous, 8N1, 9600
    uart_init(UART_BAUD_SELECT(9600, F_CPU));
    // Initialize sensors
    hum_init();
    level_sens_init();
    // Enables interrupts by setting the global interrupt mask
    sei();
    
    uint8_t key_press = 0;
    display_menu(0);
    // Infinite loop
    while (1) {
        TIM2_routine();
        TIM1_routine();
        key_press = key_press_detect(&ADC_key_value);
        if (key_press) {
            TIM2_display_timer = 0;
            char str[1] = "0";
            itoa(key_press, str, 10);
            uart_puts("\n\rKey pressed: ");
            uart_puts(str);
            display_menu(key_press);
            TIM2_display_timer = 0;
        }
        if(TIM2_display_timer > MENU_TIMEOUT_NUM){
            scroll_pos_shift = 255; //stop text shift
            TIM2_display_timer = MENU_TIMEOUT_NUM;
            if(display_update < 3){
                display_update = 4;
            }
            display_status();
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
ISR(TIMER1_OVF_vect)
{
    // Signal to main
    TIM1_flag++;
}
/**********************************************************************
 * Function: Timer/Counter2 overflow interrupt
 * Purpose:  Use single conversion mode and start.
 **********************************************************************/
ISR(TIMER2_OVF_vect)
{
    // Start ADC conversion
    TIM2_flag++;
    TIM2_display_timer++;
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
