
/*!
 * File name - header\global.h
 * #include <header\global.h>
 */

#ifndef HEADER_GLOBAL_H_
#define HEADER_GLOBAL_H_

#include "pico/stdlib.h"

extern repeating_timer_t main_repeating_timer;
extern uint32_t alarm_timer_value;
extern int32_t bme280_temperature;
extern uint32_t bme280_pressure;
extern uint32_t bme280_humidity;
extern uint16_t hot_water_common;
extern uint16_t cold_water_common;
extern uint16_t hot_water_consumption;
extern uint16_t cold_water_consumption;
extern int16_t hot_water_temp;
extern int16_t cold_water_temp;
extern uint16_t gpio_level_pwm_wrap;
extern int8_t curr_encoder_value;
extern uint8_t ds18b20_addr[4][8];
extern uint8_t nec_received_frame;
extern uint8_t hot_water_liters;
extern uint8_t cold_water_liters;
extern uint8_t display_set_lingth;
extern bool encoder_key_pressed;
extern bool repeat_timer_execute;
extern bool device_power_status;
extern bool running_sys_device;
extern bool infrared_sens_received;
extern bool update_enc_interrupt;
extern bool light_sensor_present;
extern bool rtc_alarm_executed;
extern bool frequency_time_divider;

/*!
 *
*/
extern union {
	uint64_t qwowd_data[2];
	struct {
		uint32_t dword_data[4];
	};
	struct {
		uint16_t word_data[8];
	};
	struct {
		uint8_t byte_data[16];
	};
} pn532_sector_dat;


/*!
 *
 */
bool usb_device_get_power_config(void);

/*!
 *
 */
void on_exit_to_main_cycle(void);

/*!
 *
 * @param buffer
 * @param size
 * @return
 */
uint8_t calc_dallas_crc8(uint8_t *buffer, uint16_t size);

/*!
 *
 * @param buffer
 * @param size
 * @return
 */
uint32_t calc_crc32_mpeg2(uint8_t *buffer, uint16_t size);

/*!
 *
 * @param buffer
 * @param size
 * @return
 */
uint16_t calc_crc16_modbus(uint8_t *buffer, uint16_t size);

/*!
 *
 * @param source
 * @param multiplier
 * @param numerator
 * @param denominator
 */
void convert_float_temperature(float source, float multiplier, int32_t *numerator, uint8_t *denominator);
#endif
