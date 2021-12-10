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

//static uint8_t MENU_TEXT_SIZE = MENU_TEXT_SIZE_D;

const char *menu_entries[] = {
    "Irrigation mode:",
    "Days away:",
    "Irrigation level:",
    "TEST"
};

uint16_t ADC_key_value = 65535;
uint8_t menu_pos = 0;
uint8_t scroll_pos = 0;

uint8_t slice_start = 0;
const char *shift_string;
char shift_buffer[MENU_TEXT_SIZE_D + 1] = "         \x01"; // last character is left arrow + end of string
uint8_t scroll_pos_shift = 255;

uint8_t TIM2_flag = 0;

/* Function definitions ----------------------------------------------*/

void slice_str(const char *str, char *buffer, uint8_t start)
{
    strncpy(buffer, str + start, MENU_TEXT_SIZE_D - 1);
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


void menu(uint8_t key_press)
{
    if(key_press == 3){
        scroll_pos++;
        if((menu_pos < ARRAY_SIZE(menu_entries)-2) & (scroll_pos == 2)){
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
    lcd_putc(0);
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
    itoa(MENU_TEXT_SHIFT_NUM, str, 10);
    lcd_puts(str);
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
