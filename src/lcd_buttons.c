#include <avr/io.h>         // AVR device-specific IO definitions
#include <util/atomic.h>    // Atomic operations


uint8_t key_pressed = 5;
uint8_t key_times = 0;

uint8_t key_press_detect_deb(uint8_t button, uint16_t *ADC_key_value){
    *ADC_key_value = 65535;
    if(button != key_pressed){
        key_pressed = button;
        key_times = 0;
        return 0;
    }
    key_times += 1;
    if(key_times == 2){         // debouncing
        return button;
    }
    else if(key_times == 34){   // 16.384ms*32 = 524ms
        key_times = 18;         // 16.384ms*16 = 262ms
        return button;
    }
    return 0;
}

/**********************************************************************
 * Function: Function that returns what key on display shield was pressed
 * Purpose: Return what key was pressed from ADC readed value.
 * Returns:  t_uint8 - 0 (none), 1 (SELECT), 2 (LEFT), 3 (DOWN), 4 (UP),
 *          5 (RIGHT)
 **********************************************************************/
uint8_t key_press_detect(uint16_t *ADC_key_value){
    uint16_t value;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        value = *ADC_key_value;
    }
    if (value == 65535){            // compare only actual data
        return 0;
    }
    else if ((value <= 1024) && (value > 950)) {
        key_pressed = 0;
        key_times = 0;
        *ADC_key_value = 65535;
        return 0;
    } 
    else if ((value <= 950) && (value > 650)) {
        return key_press_detect_deb(1, ADC_key_value); //SELECT
    }
    else if((value <= 650) && (value > 440)){
        return key_press_detect_deb(2, ADC_key_value); //LEFT
    }
    else if((value <= 440) && (value > 180)){
        return key_press_detect_deb(3, ADC_key_value); //UP
    }
    else if((value <= 180) && (value > 60)){
        return key_press_detect_deb(4, ADC_key_value); //DOWN
    }
    else if(value <= 60){
        return key_press_detect_deb(5, ADC_key_value); //RIGHT
    }
    return 0;
}
