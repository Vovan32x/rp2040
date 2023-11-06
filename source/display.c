
/*!
 * File name - source\display.c
 * #include <header\display.h>
 */

#include <stdio.h>
#include <header\display.h>
#include <header\global.h>
#include "pico/unique_id.h"
#include "hardware/flash.h"
#include "pico/unique_id.h"
#include "pico/rand.h"

void display_toggle_enable(uint8_t value) {
	sleep_us(420);
    uint8_t transmit = value | 0x04;
    i2c_write_blocking(DISPLAY_I2C_BUS, DISPLAY_DEFAULT_ADDRESS, &transmit, 1, false);
    sleep_us(420);
    transmit = value & ~0x04;
    i2c_write_blocking(DISPLAY_I2C_BUS, DISPLAY_DEFAULT_ADDRESS, &transmit, 1, false);
    sleep_us(420);
}

bool display_set_blink_cursor(bool blink_cursor) {
	return display_send_byte(0b00001100 | (uint)blink_cursor, false);
}

bool display_send_byte(uint8_t value, bool send_char) {
    uint8_t transmit = (uint8_t)send_char | (value & 0xF0) | (((display_set_lingth > 0) && device_power_status) ? 8 : 0);
    bool send_byte_result = (i2c_write_blocking(DISPLAY_I2C_BUS, DISPLAY_DEFAULT_ADDRESS, &transmit, 1, false) == 1);
    if (send_byte_result) {
    	display_toggle_enable(transmit);
    	transmit = (uint8_t)send_char | ((value << 4) & 0xF0) | (((display_set_lingth > 0) && device_power_status) ? 8 : 0);
    	i2c_write_blocking(DISPLAY_I2C_BUS, DISPLAY_DEFAULT_ADDRESS, &transmit, 1, false);
    	display_toggle_enable(transmit);
    }
    update_enc_interrupt = (display_set_lingth > 0);
    return send_byte_result;
}

bool lcd_display_init(void) {
	bool display_init_result = display_send_byte(0b00000011, false);
	if (display_init_result) {
		display_init_result &= display_send_byte(0b00000011, false);
		display_init_result &= display_send_byte(0b00000010, false);
		display_init_result &= display_send_byte(0b00000110, false);
		display_init_result &= display_send_byte(0b00101100, false);
		display_init_result &= display_set_blink_cursor(false);
		display_clear();
	}
	return display_init_result;
}

bool display_set_cursor(uint8_t line, uint8_t position) {
    return display_send_byte(((line == 0) ? (0x80 + position) : (0xC0 + position)), false);
}

void display_send_string(const char *string) {
    while (*string)
    	if (!display_send_byte(*string++, true))
    		break;
}

void display_clear(void) {
	if (display_send_byte(0b00000001, false))
		sleep_us(200);
}

void welcome_procedure_display(void) {
	uint8_t message[17] = { 0 };
	display_send_string("Board RP2040 UID");
	pico_unique_board_id_t unique_board;
	display_set_cursor(1, 0);
	pico_get_unique_board_id(&unique_board);
	sprintf(&message[0], "%02X%02X%02X%02X%02X%02X%02X%02X", unique_board.id[0], unique_board.id[1],
		unique_board.id[2], unique_board.id[3], unique_board.id[4],
		unique_board.id[5], unique_board.id[6], unique_board.id[7]);
	display_send_string(&message[0]);
	sleep_ms(5500);
	display_clear();
	sleep_ms(500);
	flash_get_jedec_id(&unique_board.id[0]);
	sprintf(&message[0], "MD#%02X MT#%02X C#%02X", unique_board.id[0], unique_board.id[1], unique_board.id[2]);
	display_send_string(&message[0]);
	union {
		uint32_t cpuid;
		struct {
			uint8_t byte_id[4];
		};
	} uni_cpuid;
	uni_cpuid.cpuid = rp2040_cpuid();
	display_set_cursor(1, 0);
	sprintf(&message[0], "P#%02X%02X%02X%02XB%1d", uni_cpuid.byte_id[3], uni_cpuid.byte_id[2],
		uni_cpuid.byte_id[1], uni_cpuid.byte_id[0], rp2040_chip_version());
	display_send_string(&message[0]);
	display_set_cursor(1, 13);
	sleep_ms(1000);
	for (uint8_t indx = 0; indx < 3; indx++) {
		display_send_byte('>', true);
		sleep_ms(500);
	}
	sleep_ms(1000);
	display_set_cursor(1, 13);
	for (uint8_t indx = 0; indx < 3; indx++) {
		display_send_byte('-', true);
		sleep_ms(500);
	}
	sleep_ms(500);
	display_clear();
	display_set_cursor(0, 3);
	display_send_string("Session $#");
	display_set_cursor(1, 0);
	union {
		uint64_t rand_value;
		struct {
			uint8_t byte_rand[8];
		};
	} random_value;
	random_value.rand_value = get_rand_64();
	sprintf(&message[0], "%02X%02X%02X%02X%02X%02X%02X%02X", random_value.byte_rand[0], random_value.byte_rand[1],
			random_value.byte_rand[2], random_value.byte_rand[3], random_value.byte_rand[4],
			random_value.byte_rand[5], random_value.byte_rand[6], random_value.byte_rand[7]);
	display_send_string(&message[0]);
	sleep_ms(5500);
	display_clear();
	sleep_ms(500);
}
