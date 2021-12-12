/**
 * @file hum_sens.h
 * @author Pavel Vanek (xvanek39@vutbr.cz)
 * @brief Consist rutines to work with soil humidity sensor.
 * @version 0.1
 * @date 2021-12-12
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <avr/io.h>         // AVR device-specific IO definitions
/**
 * @brief GPIO definitions
 * 
 */
#define hum_sensor PC5
#define hum_sensor_PORT PORTC
#define hum_sensor_DDR DDRC

#define max_val 450
#define min_val 30


/**
 * @brief Initialization GPIO for soil humidity sensor
 * 
 */
void hum_init(){
    hum_sensor_DDR &= ~(1<<hum_sensor); //set as input
    hum_sensor_PORT |= (1<<hum_sensor); //pullup on
}
/**
 * @brief Function read raw analog data from hum_sensor pin
 * 
 * @return uint16_t 0-1024
 */
uint16_t read_adc(){
    uint8_t ADCSRA_backup = ADCSRA;
    uint8_t ADMUX_backup = ADMUX;
    //reference from AREF pin; Multiplexor set for PC5
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
    return adc_val;
}
/**
 * @brief function return coresponding humidity
 * due to min_val and max_val
 * 
 * @param val readed analog val from ADC
 * @return long 0-100% soil humidity
 */
long to_percent(long val){
    //limiting extrem values
    if(val >= max_val) return 100;
    if(val <= min_val) return 0;
    //map from 0-100 percent
    return (val - min_val) * (100) / (max_val - min_val);
}
/**
 * @brief Function read value from ADC and recalculate to percent
 * consist functions:- long to_percent(long val)
 *                   - uint16_t read_adc()
 * @return uint8_t 0-100% adecvate soil humidity level
 */
uint8_t read_hum(){
    return to_percent(read_adc());

}



void lcd_print(uint16_t num, uint8_t x, uint8_t y, uint8_t base){
  lcd_gotoxy(x,y);
  char str[10] = "";
  itoa(num, str, base);
  lcd_puts(str);
} 


void lcd_print_str(char str[], uint8_t x, uint8_t y){
  lcd_gotoxy(x,y);
  lcd_puts(str);
} 
