
/*!
 * File name - source\global.c
 * #include <header\global.h>
 */

#ifndef HEADER_GLOBAL_C_
#define HEADER_GLOBAL_C_

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include <header\display.h>
#include <header\devtime.h>
#include "hardware\structs\usb.h"
#include "hardware\resets.h"

repeating_timer_t main_repeating_timer;
uint32_t alarm_timer_value;
int32_t bme280_temperature;
uint32_t bme280_pressure;
uint32_t bme280_humidity;
uint16_t hot_water_common;
uint16_t cold_water_common;
uint16_t hot_water_consumption;
uint16_t cold_water_consumption;
int16_t hot_water_temp;
int16_t cold_water_temp;
uint16_t gpio_level_pwm_wrap;
int8_t curr_encoder_value;
uint8_t ds18b20_addr[4][8];
//uint8_t auth_key_data[6u];
uint8_t nec_received_frame;
uint8_t hot_water_liters;
uint8_t cold_water_liters;
uint8_t display_set_lingth;
bool encoder_key_pressed;
bool repeat_timer_execute;
bool device_power_status;
bool running_sys_device;
bool infrared_sens_received;
bool update_enc_interrupt;
bool light_sensor_present;
bool rtc_alarm_executed;
bool frequency_time_divider;

union {
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

bool usb_device_get_power_config(void) {
	bool device_usb_status = !(usb_hw->muxing & (USB_USB_MUXING_TO_PHY_BITS | USB_USB_MUXING_SOFTCON_BITS));
	if (device_usb_status) {
		reset_block(RESETS_RESET_USBCTRL_BITS);
		unreset_block_wait(RESETS_RESET_USBCTRL_BITS);
		usb_hw->sie_ctrl = USB_SIE_CTRL_TRANSCEIVER_PD_BITS;
		usb_hw->pwr = USB_USB_PWR_VBUS_DETECT_BITS | USB_USB_PWR_VBUS_DETECT_OVERRIDE_EN_BITS;
		usb_hw->muxing = USB_USB_MUXING_TO_PHY_BITS | USB_USB_MUXING_SOFTCON_BITS;
	}
	return (usb_hw->sie_status & USB_SIE_STATUS_VBUS_DETECTED_BITS);
}

uint8_t calc_dallas_crc8(uint8_t *buffer, uint16_t size) {
	uint8_t crc_result = 0;
	for (uint16_t index = 0; index < size; index++) {
		uint8_t data = buffer[index];
		for (uint8_t bytes = 8; bytes > 0; bytes--) {
			crc_result = ((crc_result ^ data) & 1) ? (crc_result >> 1) ^ 0x8C : (crc_result >> 1);
			data >>= 1;
		}
	}
	return crc_result;
}

uint16_t calc_crc16_modbus(uint8_t *buffer, uint16_t size) {
	uint16_t crc_result = 0xFFFF;
	while (size--) {
		crc_result ^= *buffer++ << 8;
		for (uint8_t index = 0; index < 8; index++)
			crc_result = ((crc_result & 0x8000) ? ((crc_result << 1) ^ 0x4B37) : (crc_result << 1));
	}
	return crc_result;
}

uint32_t calc_crc32_mpeg2(uint8_t *buffer, uint16_t size) {
	uint32_t crc_table[256];
	uint32_t crc_result;
	for (uint16_t index = 0; index < 256; index++) {
		crc_result = index;
		for (uint8_t bytes = 0; bytes < 8; bytes++)
			crc_result = ((crc_result & 1) ? ((crc_result >> 1) ^ 0x04C11DB7) : (crc_result >> 1));
        crc_table[index] = crc_result;
    };
	crc_result = 0xFFFFFFFF;
	while (size--)
		crc_result = crc_table[(crc_result ^ *buffer++) & 0xFF] ^ (crc_result >> 8);
    return crc_result;
}

void on_exit_to_main_cycle(void) {
	uint8_t lcd_message[17] = { 0 };
	display_set_blink_cursor(false);
	display_clear();
	datetime_t device_current_time;
	rtc_get_datetime(&device_current_time);
	display_send_date(device_current_time.dotw, device_current_time.day, device_current_time.month, device_current_time.year);
	display_set_cursor(1, 1);
	sprintf(&lcd_message[0], "*- %02d:%02d:%02d -*", device_current_time.hour, device_current_time.min, device_current_time.sec);
	display_send_string(&lcd_message[0]);
	encoder_key_pressed = false;
	curr_encoder_value = 0;
}

void convert_float_temperature(float source, float multiplier, int32_t *numerator, uint8_t *denominator) {
	float temp_convert_value = source * multiplier;
	*numerator = (int32_t)temp_convert_value;
	temp_convert_value = (temp_convert_value - (float)*numerator) * 10.0;
	if (temp_convert_value < 0.0)
		temp_convert_value = -temp_convert_value;
	*denominator = (uint8_t)temp_convert_value;
	temp_convert_value = (temp_convert_value - (float)*denominator) * 10.0;
	if (temp_convert_value >= 5.0)
		*denominator = *denominator + 1;
if (*denominator >= 10) {
		*numerator = *numerator + 1;
		*denominator = 0;
	}
}

#endif
