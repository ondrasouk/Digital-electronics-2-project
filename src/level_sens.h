#ifndef LEVEL_SENS_H
#define LEVEL_SENS_H
/**
 * @file level_sens.h
 * @author your Vanek Pavel (xvanek39@vutbr.cz)
 * @brief This file is made as library for simple water-level sensing.
 * Individual level in water tank is sensed by 5 resistant sensors (wires).
 * Library contains functions for initialising and reading sensors.
 * @version 0.1
 * @date 2021-12-11
 * 
 * @copyright Copyright (c) 2021
 * 
 */


/**
 * @brief Definitions of used pins to connect
 * water-level sensors.
 * 
 */
#define sensor1_port &PORTC //lowest watter sensor
#define sensor1_gpio  1
#define sensor2_port &PORTC
#define sensor2_gpio  2
#define sensor3_port &PORTC
#define sensor3_gpio  3
#define sensor4_port &PORTC
#define sensor4_gpio  4
#define sensor5_port &PORTD //higest watter sensor
#define sensor5_gpio  2
//will REDO in future
#define sensor1_pin PINC
#define sensor2_pin PINC
#define sensor3_pin PINC
#define sensor4_pin PINC
#define sensor5_pin PIND

#define sensors_data_mask 0b00011111 // mask for data byte
// includes --------------------------------------------------------
#include <avr/io.h>         // AVR device-specific IO definitions

/**
 * @brief Function initialise GPIO for water-level sensors
 * all sensors are sat as INPUT with PULLUP resistor
 * 
 */
void level_sens_init (void){
    //GPIO SFR set
    *(sensor1_port-1) &= ~(1<<sensor1_gpio); // DDR set as input
    *(sensor2_port-1) &= ~(1<<sensor2_gpio); 
    *(sensor3_port-1) &= ~(1<<sensor3_gpio); 
    *(sensor4_port-1) &= ~(1<<sensor4_gpio); 
    *(sensor5_port-1) &= ~(1<<sensor5_gpio); 

    *sensor1_port |= (1<<sensor1_gpio); // set pullup resistor
    *sensor2_port |= (1<<sensor2_gpio); 
    *sensor3_port |= (1<<sensor3_gpio); 
    *sensor4_port |= (1<<sensor4_gpio); 
    *sensor5_port |= (1<<sensor5_gpio); 
}


/**
 * @brief Function read all water-sensors and compute adecvate watter-level.
 * Function also checks for degradation of sensor(in case of corosion). In case that sensors are not flooded
 * sequtialy, function returns value 255 as ERROR state.
 * For reading without checking error state use force_read_level();
 * @return uint8_t actual water level in percent (0 to 100) (empty - full) or 255 - ERROR state
 */
uint8_t read_level(void){
    uint8_t data = 0; //init input byte
    uint8_t n=0; //level counter

    // mapping GPIO bits to one byte
    // *(PORTx-2) is pointer to PINx SFR
    data |= (((sensor1_pin>>sensor1_gpio) & 0x01)<<0);
    data |= (((sensor2_pin>>sensor2_gpio) & 0x01)<<1);
    data |= (((sensor3_pin>>sensor3_gpio) & 0x01)<<2);
    data |= (((sensor4_pin>>sensor4_gpio) & 0x01)<<3);
    data |= (((sensor5_pin>>sensor5_gpio) & 0x01)<<4);
    // invert data
    data = data^sensors_data_mask;
    //count flooded sensors
    while(data & (1<<n)){ //test most right bit
        data = data ^ (1<<n); //clear tested bit
        n++; //increment level
    }
    //check for sequence
    if (!data){
        return n*20; //return number of active sensors
    }else {
        return 255; //error
    }
}
/**
 * @brief Function read all watter-sensors and ignore error state
 * 
 * @return uint8_t adecvate water-level (0 to 5) (empty - full)
 */
uint8_t force_read_level(void){
    uint8_t data = 0; //init input byte
    uint8_t n=0; //level counter

    // mapping GPIO bits to one byte
    data |= (((sensor1_pin>>sensor1_gpio) & 0x01)<<0);
    data |= (((sensor2_pin>>sensor2_gpio) & 0x01)<<1);
    data |= (((sensor3_pin>>sensor3_gpio) & 0x01)<<2);
    data |= (((sensor4_pin>>sensor4_gpio) & 0x01)<<3);
    data |= (((sensor5_pin>>sensor5_gpio) & 0x01)<<4);
    //invert data
    data = data^sensors_data_mask;
    //count flooded sensors
    while(data & (1<<n)){ //test most right bit
        data = data ^ (1<<n); //clear tested bit
        n++; //increment level
    }
    return n;
}
/* 
void my_print2(uint8_t num){
    char str[8] = "";
    itoa(num, str, 2);
    uart_puts("\n\r ");
    uart_puts(str);
}
 */
#endif
