#ifndef LCD_BUTTONS
#define LCD_BUTTONS

/* Includes ----------------------------------------------------------*/
#include <avr/io.h>
#include <util/atomic.h>

uint8_t key_press_detect_deb(uint8_t button, uint16_t *ADC_key_value);

uint8_t key_press_detect(uint16_t *ADC_key_value);

#endif
