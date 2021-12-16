/*
 * DS RTC Library: DS1307 and DS3231 driver library
 * (C) 2011 Akafugu Corporation
 *
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 */
 
 /**
  * @ingroup rtc
  * @file rtc.h
  * @brief Consist rutines to work with RTC.
  * @author Akafugu Corporation
  * @date 2011
  */ 

#ifndef DS1307_H
#define DS1307_H

#include <stdbool.h>
#include <avr/io.h>
#include "twi.h"

#define DS1307_SLAVE_ADDR 0b11010000

/** Time structure
 * 
 * Both 24-hour and 12-hour time is stored, and is always updated when rtc_get_time is called.
 * 
 * When setting time and alarm, 24-hour mode is always used.
 *
 * If you run your clock in 12-hour mode:
 * - set time hour to store in twelveHour and set am to true or false.
 * - call rtc_12h_translate (this will put the correct value in hour, so you don't have to
 *   calculate it yourself.
 * - call rtc_set_alarm or rtc_set_clock
 *
 * Note that rtc_set_clock_s, rtc_set_alarm_s, rtc_get_time_s, rtc_set_alarm_s always operate in 24-hour mode
 * and translation has to be done manually (you can call rtc_24h_to_12h to perform the calculation)
 *
 */
 /**
  * @ingroup rtc
  * @defgroup obj   Objects
  */
  
  /**
   * @ingroup obj
   * @brief Structure of time variables
   * @details variables for seconds. minutes, hours, day in month, months, years, day in week and switch for 12/24 cycle
   */ 
struct tm {
	int sec;      // 0 to 59
	int min;      // 0 to 59
	int hour;     // 0 to 23
	int mday;     // 1 to 31
	int mon;      // 1 to 12
	int year;     // year-99
	int wday;     // 1-7

    // 12-hour clock data
    bool am; // true for AM, false for PM
    int twelveHour; // 12 hour clock time
};

// statically allocated 
extern struct tm _tm;

/**
 * @ingroup rtc
 * @brief Initialize the RTC and autodetect type (DS1307 or DS3231)
 * @details Attempt autodetection:
	       1) Read and save temperature register
	       2) Write a value to temperature register
	       3) Read back the value
	       equal to the one written: DS1307, write back saved value and return
 * @return 0 if communication is ok, 1 if fail
 */
uint8_t rtc_init(void);
/**
 * @ingroup rtc
 * @brief set type of RTC to 1307
 */ 
void rtc_set_ds1307(void);
/**
 * @ingroup rtc
 * @brief set type of RTC to 3231
 */ 
void rtc_set_ds3231(void);

// Get/set time
// Gets the time: Supports both 24-hour and 12-hour mode
/**
 * @ingroup rtc
 * @brief Function to get time from RTC
 * @details function supports both 24-hour and 12-hour mode
 * @details read 7 bytes starting from register 0 (sec, min, hour, day-of-week, date, month, year)
 * @details Clear clock halt bit from read data
 * @return structure tm filled with time variables
 */ 
struct tm* rtc_get_time(void);
// Gets the time: 24-hour mode only
/**
 * @ingroup rtc
 * @brief Function to get time from RTC
 * @details function supports just 24-hour mode
 * @details read just hours, minutes and seconds
 * @param hour variable where to store red hour value
 * @param min variable where to store red minutes value
 * @param sec variable where to store red seconds value
 */
void rtc_get_time_s(uint8_t* hour, uint8_t* min, uint8_t* sec);
// Sets the time: Supports both 24-hour and 12-hour mode
/**
 * @ingroup rtc
 * @brief Function to set time to RTC
 * @details function supports both 24-hour and 12-hour mode
 * @details set 7 bytes starting from register 0 (sec, min, hour, day-of-week, date, month, year) 
 * @param tm_ type tm, structure to store time values
 */ 
void rtc_set_time(struct tm* tm_);
// Sets the time: Supports 12-hour mode only
/**
 * @ingroup rtc
 * @brief Function to set time to RTC
 * @details function supports just 12-hour mode
 * @details set just hours, minutes and seconds 
 * @param hour variable where to find what should be written in rtc
 * @param min variable where find what should be written in rtc
 * @param sec variable where find what should be written in rtc
 */
void rtc_set_time_s(uint8_t hour, uint8_t min, uint8_t sec);

// start/stop clock running (DS1307 only)
/**
 * @ingroup rtc
 * @brief start/stop clock run (DS1307 only)
 */
void    rtc_run_clock(uint8_t run);
/**
 * @ingroup rtc
 * @brief Check if clock run
 * 
 */ 
uint8_t rtc_is_clock_running(void);


// SRAM read/write DS1307 only
/**
 * @ingroup rtc
 * @brief function reads value of whole 56 bytes
 * @details because it's impossible to read that much in one, function calls rtc_get_sram_byte and saves it in structure
 * @param data structure where to store red data
 */
void rtc_get_sram(uint8_t* data);
/**
 * @ingroup rtc
 * @brief function sets value of whole 56 bytes
 * @details because it's impossible to read that much in one, function calls rtc_gset_sram_byte and sets it gradually
 * @param data structure where to find data for setting
 */
void rtc_set_sram(uint8_t *data);
/**
 * @ingroup rtc
 * @brief function reads value to relevant byte
 * @param offset register id
 */
uint8_t rtc_get_sram_byte(uint8_t offset);
/**
 * @ingroup rtc
 * @brief function writes value to relevant byte
 * @param b value of time variable
 * @param offset register id
 */
void rtc_set_sram_byte(uint8_t b, uint8_t offset);

// Auxiliary functions
/**
 * @ingroup rtc
 * @brief Auxiliary functions
 */ 
enum RTC_SQW_FREQ { FREQ_1 = 0, FREQ_1024, FREQ_4096, FREQ_8192 };
/**
 * @ingroup rtc
 * @brief Auxiliary functions
 */
void rtc_SQW_enable(bool enable);
/**
 * @ingroup rtc
 * @brief Auxiliary functions
 */
void rtc_SQW_set_freq(enum RTC_SQW_FREQ freq);
/**
 * @ingroup rtc
 * @brief Auxiliary functions
 */
void rtc_osc32kHz_enable(bool enable);

// Alarm functionality

/**
 * @ingroup rtc
 * @brief Function for reset alarm
 */
void rtc_reset_alarm(void);
/**
 * @ingroup rtc
 * @brief set alarm to timeesaved in structure tm
 * @param tm_ structure for storing time variables  
 */
void rtc_set_alarm(struct tm* tm_);
/**
 * @ingroup rtc
 * @brief auxiliary function for rtc_set_alarm
 * @details function call another functions and give them variable value and number of pin
 * @param hour variable from structure tm_
 * @param min   variable from structure tm_
 * @param sec   variable from structure tm_
 */
void rtc_set_alarm_s(uint8_t hour, uint8_t min, uint8_t sec);
/**
 * @ingroup rtc
 * @brief  function for reading time from alarm
 * @details function fall rtc_get_alarm_s to get time values and store it in tm structur
 */ 
struct tm* rtc_get_alarm(void);
/**
 * @ingroup rtc
 * @brief  function reads data from rtc and save them in given variables
 * @param hour pointer for hour variable
 * @param min pointer for minute variable
 * @param sec pointer for second variable
 */
void rtc_get_alarm_s(uint8_t* hour, uint8_t* min, uint8_t* sec);
/**
 * @ingroup rtc
 * @brief check alarm
 * @derails function reads values of rtc and when they match sets data, it returns true, else if returns false
 * @return true when alarm finishes, false when alarm rums
 */ 
bool rtc_check_alarm(void);  

#endif
