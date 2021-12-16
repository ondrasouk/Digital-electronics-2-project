#ifndef LCD_BUTTONS
#define LCD_BUTTONS

/**
 * @file lcd_buttons.h
 * @author Onrej Soukenik
 * @date 16.12.2021
 * @brief Consist functions for buttons operation

/* Includes ----------------------------------------------------------*/
#include <avr/io.h>
#include <util/atomic.h>
/**
 * @ingroup lcd
 * @brief Function for button debouncing
 * @detail When button is pressed and and key_press_detection function decide a value, parametres are passed here
 * @detail Function checks valid duration of pressed key impulse
 * @return 0 if invalid; number of key if valid 
 */ 
uint8_t key_press_detect_deb(uint8_t button, uint16_t *ADC_key_value);
/**
 * @ingroup lcd
 * @brief Function for button detection
 * @detail function decide which button was pressed according to ADC value
 * @param ADC_key_value value from keys
 * @return Returns pressed key - 1-5 or none
 * @detail 0 (none), 1 (SELECT), 2 (LEFT), 3 (DOWN), 4 (UP), 5 (RIGHT)
 */ 
uint8_t key_press_detect(uint16_t *ADC_key_value);

#endif
