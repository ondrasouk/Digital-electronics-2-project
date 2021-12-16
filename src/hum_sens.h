#ifndef HUM_SENS_H
#define HUM_SENS_H
/**
 * @ingroup hum
 * @file hum_sens.h
 * @author Pavel Vanek (xvanek39@vutbr.cz)
 * @brief Consist rutines to work with soil humidity sensor.
 * Sensor is working as resistive voltage divider.
 * During idle state current flowing thrue probe must be minimalised to prevent
 * corosion due  electrolysis.
 * 
 * 
 * @version 0.1
 * @date 2021-12-12
 *
 * @copyright Copyright (c) 2021
 * 
 */
#include <avr/io.h> // AVR device-specific IO definitions
#include <util/delay.h>
#include <avr/interrupt.h>  // Interrupts standard C library for AVR-GCC
/**
 * @name GPIO humidity sensor definitions definitions
 */
#define hum_sensor PC3
#define hum_sensor_PORT PORTC
#define hum_sensor_DDR DDRC
/**
 * @name Calibration ADC values for soil humidity transversion
 */
#define max_val 450 /** < ADC equal value for 0% */
#define min_val 30  /** < ADC equal value for 100% */


/**
 * @brief Initialization GPIO for soil humidity sensor
 *
 */
void hum_init(){
    hum_sensor_DDR &= ~(1<<hum_sensor); /**set as input*/
    hum_sensor_PORT &= ~(1<<hum_sensor); /**pullup off*/
}
/**
 * @ingroup hum
 * @brief  Function for main operation with humidity sensor
 * @detail Function read raw analog data from hum_sensor pin.
 * @detail Function also automaticaly turns off pullup resistor to prevent corosion of probec due electrolysis.
 * 
 * @return uint16_t readed ADC value (0-1024)
 */
uint16_t read_adc(){
    //pullup on (anti-corosion protection)
    hum_sensor_PORT |= (1<<hum_sensor);
    //disable global interrupts
    cli();
    //store actual used ADC setting
    uint8_t ADCSRA_backup = ADCSRA;
    uint8_t ADMUX_backup = ADMUX;
    //reference from AREF pin; Multiplexor set for PC5
    ADMUX = 0x00;
    ADCSRA = 0x00;
    ADMUX = (1<<REFS0)| hum_sensor;
    //enable ADC, prescaler 128, start conversion
    ADCSRA = (1<<ADEN)|(1<<ADSC)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //SET ADC SFR
    //wait for complete conversion (pooling)
    while ((ADCSRA&(1<<ADIF))==0){
        _delay_us(1);
    }
    // read adc value
    uint16_t adc_val = ADC;
    //restore original ADC setting
    ADMUX = ADMUX_backup;
    ADCSRA = ADCSRA_backup;
    //enable global interrupts
    sei();
    //pullup off (anti-corosion protection)
    hum_sensor_PORT &= ~(1<<hum_sensor); 

    return adc_val;
}

/**
 * @ingroup hum
 * @brief Function for transfering raw value to homidity in %
 * @detail function return coresponding humidity due to min_val and max_val
 *
 * @param val readed analog val from ADC
 * @return long 0-100% soil humidity
 */
uint8_t to_percent(uint16_t val){
    //limiting extrem values
    if(val >= max_val) return 0;
    if(val <= min_val) return 100;
    //map from 0-100 percent
    return 100 - (val - min_val) * (100) / (max_val - min_val);
}
/**
 * @ingroup hum
 * @brief  function for easy calling from main program
 * @detail Function read value from ADC and recalculate to percent
 * consist functions:- long to_percent(long val)
 *                   - uint16_t read_adc()
 * @return uint8_t 0-100% adecvate soil humidity level
 */
uint8_t read_hum(){
    return to_percent(read_adc());
}

#endif
