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

/* Globals -----------------------------------------------------------*/
// Custom character definition using https://omerk.github.io/lcdchargen/
const uint8_t customChar[] = {
    0b00111, 0b01110, 0b11100, 0b11000, 0b11100, 0b01110, 0b00111,
    0b00011, // 0
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
#define char_array_size sizeof(menu_entries)/2
const char *menu_entries[] = {
    "Menu 1", 
    "Menu 2", 
    "Menu 3",
    "Menu 4",
    "Menu 5",
    "Menu 6",
};
uint16_t ADC_key_value = 65535;
uint8_t menu_pos = 0;
uint8_t scroll_pos = 0;

/* Function definitions ----------------------------------------------*/

void menu(uint8_t key_press)
{
    if(key_press == 3){
        scroll_pos++;
        if((menu_pos < char_array_size-2) & (scroll_pos == 2)){
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
    lcd_gotoxy(1,0);    // Fix some unkonwn problem
    lcd_puts(menu_entries[menu_pos]);
    lcd_gotoxy(1,1);
    lcd_puts(menu_entries[menu_pos+1]);
    lcd_gotoxy(0,scroll_pos);
    lcd_putc(1);
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
    itoa(char_array_size, str, 10);
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
    ADCSRA |= (1 << ADSC);
}

/**********************************************************************
 * Function: ADC complete interrupt
 * Purpose:  Copy ADC value.
 **********************************************************************/
ISR(ADC_vect) {
    ADC_key_value = ADC;  
}
