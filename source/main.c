
/*!
 * File name - source\main.c
 * #include <header\main.h>
 */

#include <math.h>
#include <header\main.h>
#include <header\display.h>
#include <header\mifare.h>
#include <header\bme280.h>
#include <header\global.h>
#include <header\interrupt.h>
#include <header\quadrature.h>
#include <header\nec_receive.h>
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "hardware/pll.h"
#include "hardware/uart.h"
#include "hardware/xosc.h"
#include "hardware/resets.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "hardware/exception.h"
#include "hardware/vreg.h"
#include "hardware/pwm.h"
#include "pico/multicore.h"
#include "boards/pico.h"
#include <header\devtime.h>

bool repeat_timer_callback(repeating_timer_t *rt) {
	if (!repeat_timer_execute) {
		if (display_set_lingth != 0)
			display_set_lingth--;
		repeat_timer_execute = true;
	}
#if defined RELEASE_PROJECT_CONFIG
	watchdog_update(7500 * 1000 * 2);
#endif
	bool temp_power_status = device_power_status;
	device_power_status = usb_device_get_power_config();
	if (temp_power_status != device_power_status) {
		if (!device_power_status) {
			gpio_level_pwm_wrap = 200;
			pwm_set_irq_enabled(PWM_SLICE_5, false);
			pwm_set_both_levels(PWM_SLICE_5, 0, 199);
			gpio_set_function(GPIO_CLK_PIN_21, GPIO_FUNC_NULL);
			display_send_byte(0b00001100, false);
			pwm_set_enabled(PWM_SLICE_5, false);
		} else {
			pwm_set_enabled(PWM_SLICE_5, true);
			gpio_set_function(GPIO_CLK_PIN_21, GPIO_FUNC_GPCK);
		}
	}
	if (frequency_time_divider && device_power_status) {
			clock_hw_t gpio_clock_conf = clock_gpio_get_divider(GPIO_CLK_PIN_21);
			uint32_t current_divider = (gpio_clock_conf.div >> CLOCKS_CLK_GPOUT0_DIV_INT_LSB);
			if (current_divider >= 2500) {
				current_divider = 500;
			} else {
				current_divider += 5;
			}
		clock_gpio_init(GPIO_CLK_PIN_21, CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_VALUE_CLK_USB, clock_get_hz(clk_usb) / current_divider);
	} else
		gpio_set_function(GPIO_CLK_PIN_21, GPIO_FUNC_NULL);
	frequency_time_divider = !frequency_time_divider;
	return running_sys_device;
}

void gpio_callback_handler(uint gpio, uint32_t event_mask) {
	if (event_mask == GPIO_IRQ_EDGE_FALL) {
		switch (gpio) {
			case GPIO_SIO_PIN_01: {
				display_set_lingth = 120;
				repeat_timer_execute = true;
				break;
			}
			case GPIO_SIO_PIN_15: {
				pn532_execute_interrupt();
				break;
			}
		}
	}
	if (event_mask == GPIO_IRQ_EDGE_RISE) {
		switch (gpio) {
			case GPIO_SIO_PIN_01: {
				display_set_lingth = 120;
				repeat_timer_execute = true;
				break;
			}
			case GPIO_SIO_PIN_02: {
				hot_water_consumption++;
				hot_water_liters++;
				if (hot_water_liters >= 100) {
					hot_water_liters = 0;
					hot_water_common++;
				}
				break;
			}
			case GPIO_SIO_PIN_03: {
				cold_water_consumption++;
				cold_water_liters++;
				if (cold_water_liters >= 100) {
					cold_water_liters = 0;
					cold_water_common++;
				}
				break;
			}
			case GPIO_SIO_PIN_08: {
				display_set_lingth = 120;
				if (!encoder_key_pressed)
					encoder_key_pressed = true;
				break;
			}
		}
	}
}

void on_encoder_right_main_run(void) {
	if (device_power_status) {
		int8_t current_enc_position = 4;
		bool first_line_display = true;
		switch (current_enc_position) {
			case 0x00: {
				break;
			}
			case 0x04: {
				setup_alarm_device_time();
				break;
			}
			default: {
				break;
			}
		}
		setup_alarm_device_time();
	}
}

void on_encoder_left_main_run(void) {
	if (device_power_status) {
		uint8_t display_buffer[17] = { 0 };
		int8_t current_read_sensor = 4;
		bool execute_rotate_proc = true;
		bool first_line_display = true;
		bool bme280_farengeit = false;
		curr_encoder_value = 0;
		int32_t numerator_val;
		uint8_t denumerator_val;
		uint8_t disp_char_num;
		//display_send_byte(0b00001110, false); // Только для тестирование позиции курсорв при выводе данных
		absolute_time_t wait_rotate_time = make_timeout_time_ms(60000);
		while (execute_rotate_proc && device_power_status && !time_reached(wait_rotate_time)) {
			absolute_time_t wait_read_sensor = make_timeout_time_ms(2000);
			switch (current_read_sensor) {
				/*
				case 0: {
					if (first_line_display) {
						display_clear();
						first_line_display = false;
						display_send_string("  Light sensor");
					}
					if (light_sensor_present) {
						uint16_t light_value = 0;
						if (i2c_read_blocking(BH1750_DEFAULT_BUS, BH1750_DEFAULT_ADDRESS, (uint8_t *)&light_value, 2, false) == 2) {
							display_set_cursor(1, 2);
							uint8_t disp_number = 12 - sprintf(&display_buffer[0], " %d Lx.", light_value);
							display_send_string(&display_buffer[0]);
							while (disp_number--)
								display_send_byte(' ', true);
						} else {
							light_sensor_present = false;
						}
					}
					if (!light_sensor_present) {
						display_set_cursor(1, 2);
						display_send_string("*- ERROR -*");
						bh1750_init_device();
					}
					break;
				}
				*/
				case 1: {
					if (first_line_display) {
						display_clear();
						first_line_display = false;
						display_send_string("Cold/Hot on days");
					}
					display_set_cursor(1, 1);
					sprintf(&display_buffer[0], "%d", hot_water_consumption * 10);
					display_send_string(&display_buffer[0]);
					display_set_cursor(1, 15 - sprintf(&display_buffer[0], "%d", cold_water_consumption * 10));
					display_send_string(&display_buffer[0]);
					break;
				}
				case 2: {
					if (first_line_display) {
						display_clear();
						first_line_display = false;
						display_send_string("Cold/Hot cubics.");
					}
					display_set_cursor(1, 1);
					sprintf(&display_buffer[0], "%d.%d", hot_water_common, hot_water_liters);
					display_send_string(&display_buffer[0]);
					display_set_cursor(1, 15 - sprintf(&display_buffer[0], "%d.%d", cold_water_common, cold_water_liters));
					display_send_string(&display_buffer[0]);
					break;
				}
				case 3: {
					if (first_line_display) {
						display_clear();
						first_line_display = false;
						display_send_string("Cold/Hot temper.");
					}
					display_set_cursor(1, 1);
					if (hot_water_temp >= 0) {
						convert_float_temperature(hot_water_temp, 0.0625, &numerator_val, &denumerator_val);
						disp_char_num = (10 - sprintf(&display_buffer[0], "%02d.%d", numerator_val, denumerator_val));
						display_send_string(&display_buffer[0]);
					} else {
						disp_char_num = 6;
						display_send_string("--.-");
					}
					while (disp_char_num--)
						display_send_byte(' ', true);
					if (cold_water_temp >= 0) {
						convert_float_temperature(cold_water_temp, 0.0625, &numerator_val, &denumerator_val);
						display_set_cursor(1, 15 - sprintf(&display_buffer[0], "%02d.%d", numerator_val, denumerator_val));
						display_send_string(&display_buffer[0]);
					} else {
						display_set_cursor(1, 11);
						display_send_string("--.-");
					}
					break;
				}
				case 4: {
					if (first_line_display) {
						display_clear();
						bme280_farengeit = false;
						first_line_display = false;
						display_send_string("Air");
					}
					float bme280_temp_value = (float)bme280_pressure / PRESSURE_DIVISOR_HPA;
					if (bme280_temp_value < (BME280_DEFAULT_PRESSURE * PRESSURE_DIVISOR_MMH)) {
						if (!bme280_farengeit) {
							bme280_temp_value = bme280_temp_value / PRESSURE_DIVISOR_MMH;
							convert_float_temperature(bme280_temp_value, 1.0, &numerator_val, &denumerator_val);
						} else {
							convert_float_temperature(bme280_temp_value, 0.01, &numerator_val, &denumerator_val);
						}
						disp_char_num = sprintf(&display_buffer[0], "%03d.%d %s", numerator_val, denumerator_val, (!bme280_farengeit) ? "mmHg" : "hPa ");
						display_set_cursor(0, 16 - disp_char_num);
						display_send_string(&display_buffer[0]);
						disp_char_num = (16 - 3) - disp_char_num;
					} else {
						disp_char_num = 4;
						display_set_cursor(0, 7);
						display_send_string("--.- unit");
					}
					display_set_cursor(0, 3);
					while (disp_char_num--)
						display_send_byte(' ', true);
					display_set_cursor(1, 0);
					bme280_temp_value = (float)bme280_humidity / HUMIDITY_DIVISOR;
					if (bme280_temp_value < BME280_DEFAULT_HUMIDITY) {
						convert_float_temperature(bme280_temp_value, 1.0, &numerator_val, &denumerator_val);
						disp_char_num = (9 - sprintf(&display_buffer[0], "%02d.%d %%", numerator_val, denumerator_val));
						display_send_string(&display_buffer[0]);
					} else {
						disp_char_num = 3;
						display_send_string("--.- %");
					}
					while (disp_char_num--)
						display_send_byte(' ', true);
					bme280_temp_value = (float)bme280_temperature / TEMPERATURE_DIVISOR;
					if (bme280_temp_value > (BME280_DEFAULT_TEMPERATURE + 1)) {
						if (bme280_farengeit)
							bme280_temp_value = (bme280_temp_value * FARENGEIT_MULTIPLIER) + FARENGEIT_ADD_VALUE;
						convert_float_temperature(bme280_temp_value, 1.0, &numerator_val, &denumerator_val);
						display_set_cursor(1, 16 - sprintf(&display_buffer[0], "%02d.%d %c%c", numerator_val, denumerator_val, 0xDF, (!bme280_farengeit) ? 'C' : 'F'));
					} else {
						display_set_cursor(1, 16 - sprintf(&display_buffer[0], "--.- %cX", 0xDF));
					}
					display_send_string(&display_buffer[0]);
					bme280_farengeit = !bme280_farengeit;
					bme280_read_parameters(&bme280_temperature, &bme280_pressure, &bme280_humidity);
					break;
				}
				default: {
					if (current_read_sensor >= 5)
						current_read_sensor = 0;
					else
						current_read_sensor = 4;
					continue;
				}
			}
			while (!time_reached(wait_read_sensor) && device_power_status) {
				if ((curr_encoder_value != 0) || encoder_key_pressed)
					break;
			}
			if (curr_encoder_value != 0) {
				current_read_sensor += curr_encoder_value;
				curr_encoder_value = 0;
				wait_rotate_time = make_timeout_time_ms(60000);
				first_line_display = true;
			}
			if (encoder_key_pressed || !device_power_status) {
				execute_rotate_proc = false;
				encoder_key_pressed = false;
				curr_encoder_value = 0;
			}
		}
	}
	on_exit_to_main_cycle();
}

void on_encoder_keypre_main_run(void) {
	absolute_time_t wait_keypre_time;
	uint8_t current_option_press = 0;
	int8_t last_option_press = 0;
	bool execute_config_proc = true;
	void internal_config_procedure(void) {
		display_clear();
		curr_encoder_value = 0;
		encoder_key_pressed = false;
		display_send_string(" Config device.");
		display_set_cursor(1, 0);
		display_send_string("Setup");
		current_option_press = last_option_press;
		wait_keypre_time = make_timeout_time_ms(15000);
	}
	void internal_options_pn532_error(uint8_t err_num) {
		pn5532_deactivate();
		display_clear();
		uint8_t display_buffer[17];
		sprintf(&display_buffer[0], "Options %1d error!", err_num);
		display_send_string(&display_buffer[0]);
		display_set_cursor(1, 0);
		display_send_string(" ***  Exit  *** ");
		sleep_ms(5000);
		current_option_press = 0;
	}
	if (device_power_status) {
		internal_config_procedure();
		while (execute_config_proc && device_power_status && !time_reached(wait_keypre_time)) {
			switch (current_option_press) {
				case 0: { // (0 + 1) * 4 = 4
					display_set_cursor(1, 6);
					display_send_string("date/time.");
					current_option_press = 64;
					break;
				}
				case 1: { // (1 + 1) * 4 = 8
					display_set_cursor(1, 6);
					display_send_string("options 1.");
					current_option_press = 64;
					break;
				}
				case 2: { // (2 + 1) * 4 = 12
					display_set_cursor(1, 6);
					display_send_string("options 2.");
					current_option_press = 64;
					break;
				}
				case 3: { // (3 + 1) * 4 = 16
					display_set_cursor(1, 6);
					display_send_string("exit conf.");
					current_option_press = 64;
					break;
				}
				case 4: { // Setup time
					datetime_t device_current_time;
					rtc_get_datetime(&device_current_time);
					if (setup_current_device_time(&device_current_time, 30000))
						rtc_set_datetime(&device_current_time);
					display_set_blink_cursor(false);
					current_option_press = 0;
					internal_config_procedure();
					break;
				}
				case 8: {
					if (mifare_card_auth_device(300000, true)) {
						datetime_t device_current_time;
						rtc_get_datetime(&device_current_time);
						device_current_time.sec = calc_dallas_crc8((uint8_t *)&device_current_time, sizeof(device_current_time) - 1);
						memcpy(&pn532_sector_dat.byte_data[0], &device_current_time, sizeof(device_current_time));
						pn532_sector_dat.byte_data[8] = 0;
						for (uint8_t index = 0; index < 7; index++)
							pn532_sector_dat.byte_data[8] += pn532_sector_dat.byte_data[index];
						pn532_sector_dat.byte_data[8] = pn532_sector_dat.byte_data[8] ^ 0x55;
						pn532_sector_dat.byte_data[9] = calc_dallas_crc8(&pn532_sector_dat.byte_data[0], 9);
						uint16_t modbus_crc = calc_crc16_modbus(&pn532_sector_dat.byte_data[0], 9);
						pn532_sector_dat.byte_data[10] = (uint8_t)(modbus_crc >> 8);
						pn532_sector_dat.byte_data[11] = (uint8_t)modbus_crc;
						pn532_activate();
						if (pn532_write_block(5, false)) {
							pn5532_deactivate();
							//
							current_option_press = 1;
						} else {
							pn5532_deactivate();
							internal_options_pn532_error(1);
						}
					} else {
						internal_options_pn532_error(1);
					}
					internal_config_procedure();
					break;
				}
				case 12: {
					internal_options_pn532_error(2);
					internal_config_procedure();
					break;
				}
				case 16: {
					execute_config_proc = false;
					current_option_press = 3;
					break;
				}
			}
			if (execute_config_proc && device_power_status) {
				if (curr_encoder_value != 0) {
					wait_keypre_time = make_timeout_time_ms(15000);
					last_option_press += curr_encoder_value;
					if (last_option_press >= 4)
						last_option_press = 0;
					if (last_option_press <= -1)
						last_option_press = 3;
					current_option_press = last_option_press;
					curr_encoder_value = 0;
				}
				if (encoder_key_pressed) {
					current_option_press = ((uint8_t)last_option_press + 1) * 4;
					encoder_key_pressed = false;
				}
			} else {
				curr_encoder_value = 0;
				encoder_key_pressed = false;
			}
		}
	}
	on_exit_to_main_cycle();
}

void device_main_run(void) {
	datetime_t device_time;
	int8_t old_time_day = -1;
	int8_t old_time_hrs = -1;
	int8_t old_time_min = -1;
	uint8_t lcd_message[17] = { 0 };
	bool seconds_delimeter = true;
	bool read_sensors_data = false;
	on_exit_to_main_cycle();
	while (running_sys_device) {
		rtc_get_datetime(&device_time);
		if (old_time_day != device_time.day) {
			old_time_day = device_time.day;
			display_send_date(device_time.dotw, device_time.day, device_time.month, device_time.year);
		}
		if (old_time_hrs != device_time.hour) {
			old_time_hrs = device_time.hour;
			display_set_cursor(1, 4);
			sprintf(&lcd_message[0], "%02d", device_time.hour);
			display_send_string(&lcd_message[0]);
			read_sensors_data = true;
			if (device_time.hour == 3) {
				hot_water_consumption = 0;
				cold_water_consumption = 0;
				if (device_time.day == 1) {
					hot_water_liters = 0;
					cold_water_liters = 0;
					hot_water_common = 0;
					cold_water_common = 0;
				}
			}
		}
		if (old_time_min != device_time.min) {
			old_time_min = device_time.min;
			display_set_cursor(1, 6);
			sprintf(&lcd_message[0], ":%02d", device_time.min);
			display_send_string(&lcd_message[0]);
			seconds_delimeter = true;
		}
		if (seconds_delimeter) {
			if (!device_power_status)
				display_set_blink_cursor(false);
			if (device_time.sec != 0) {
				display_set_cursor(1, 6);
				display_send_byte(':', true);
			}
			display_set_cursor(1, 9);
			sprintf(&lcd_message[0], ":%02d", device_time.sec);
			display_send_string(&lcd_message[0]);
		} else {
			if (display_set_lingth > 0) {
				if (!device_power_status) {
					display_set_lingth = 0;
					display_send_byte(0b00001000, false);
				} else {
					display_set_cursor(1, 6);
					display_send_byte(' ', true);
					display_set_cursor(1, 9);
					display_send_byte(' ', true);
				}
			}
		}
		seconds_delimeter = !seconds_delimeter;
		while (!repeat_timer_execute) {
			if (read_sensors_data && device_power_status) {
				bme280_read_parameters(&bme280_temperature, &bme280_pressure, &bme280_humidity);
				read_sensors_data = false;
			}
			if (rtc_alarm_executed) {
				display_send_alarm_time();
				old_time_hrs = -1;
				old_time_min = -1;
			}
			if (curr_encoder_value != 0) {
				if (update_enc_interrupt) {
					if (curr_encoder_value > 0)
						on_encoder_right_main_run();
					else
						on_encoder_left_main_run();
				} else {
					curr_encoder_value = 0;
				}
			}
			if (encoder_key_pressed) {
				if (update_enc_interrupt)
					on_encoder_keypre_main_run();
				else
					encoder_key_pressed = false;
			}
			if (infrared_sens_received) {
				switch (nec_received_frame) {
					case 0: {
						break;
					}
					case 1: {
						break;
					}
				}
			}
		}
		repeat_timer_execute = false;
	}
}

void deactivate_device(int exception_num) {
	rtc_stop();
#if defined RELEASE_PROJECT_CONFIG
	watchdog_disable();
#endif
	if (exception_num != USER_4_IRQ_IRQ) {
		gpio_set_function(GPIO_CLK_PIN_21, GPIO_FUNC_NULL);
		gpio_set_pulls(GPIO_CLK_PIN_21, false, true);
		gpio_set_function(GPIO_PWM5_CH_A_PIN_26, GPIO_FUNC_NULL);
		gpio_set_pulls(GPIO_PWM5_CH_A_PIN_26, false, true);
#if defined RELEASE_PROJECT_CONFIG
		pn532_sleep_mode(true);
		multicore_reset_core1();
#else
		pn532_sleep_mode(false);
#endif
		irq_set_mask_enabled(0x03FFFFF0, false);
		pio_sm_set_enabled(ENCODER_PIO, ENCODER_PIO_SM, false);
		pio_clear_instruction_memory(ENCODER_PIO);
		pio_sm_set_enabled(NEC_RECEIVE_PIO, NEC_RECEIVE_PIO_SM, false);
		pio_clear_instruction_memory(NEC_RECEIVE_PIO);
		gpio_put(GPIO_LED_PIN_25, false);
		gpio_put(GPIO_SIO_PIN_09, false);
		pwm_set_irq_enabled(PWM_SLICE_5, false);
		pwm_set_clkdiv_mode(PWM_SLICE_5, PWM_DIV_FREE_RUNNING);
		pwm_set_chan_level(PWM_SLICE_5, PWM_CHAN_A, 0);
		display_clear();
		display_set_lingth = 240;
		running_sys_device = false;
		device_power_status = false;
		light_sensor_present = false;
		pwm_set_enabled(PWM_SLICE_5, false);
		bool display_present = display_set_blink_cursor(false);
		if (display_present) {
			switch (exception_num) {
				case HARD_FAULT_IRQ: {
					display_send_string("  Hards Fault!");
					break;
				}
				case CLOCKS_IRQ_IRQ: {
					display_send_string(" SysClock fail!");
					break;
				}
				case USER_0_IRQ_IRQ: {
					display_send_string(" Access deined!");
					break;
				}
				case USER_1_IRQ_IRQ: {
					display_send_string(" Watchdog reset");
					break;
				}
				case USER_2_IRQ_IRQ: {
					display_send_string(" No mifare read");
					break;
				}
				default: {
					display_send_string(" Unknown error ");
					break;
				}
			}
			display_set_cursor(1, 1);
			display_send_string("System halted.");
		}
		for (uint8_t index = 0; index < 150; index++) {
			if (exception_num != USER_0_IRQ_IRQ)
				gpio_put(GPIO_SIO_PIN_09, device_power_status);
			if (display_present)
				display_set_blink_cursor(false);
			gpio_put(GPIO_LED_PIN_25, device_power_status);
			device_power_status = !device_power_status;
			sleep_ms(150);
		}
		if (display_present)
			display_send_byte(0b00001000, false);
	}
	gpio_put(GPIO_SIO_PIN_09, false);
	reset_block(~(
			RESETS_RESET_SYSCFG_BITS |
			RESETS_RESET_IO_QSPI_BITS |
			RESETS_RESET_PLL_USB_BITS |
			RESETS_RESET_USBCTRL_BITS |
			RESETS_RESET_PLL_SYS_BITS |
			RESETS_RESET_IO_BANK0_BITS |
			RESETS_RESET_PADS_QSPI_BITS));
	while (true) {
#if defined RELEASE_PROJECT_CONFIG
		xosc_dormant(true);
#endif
	}
}

int main(void) {
	gpio_init(PICO_SMPS_MODE_PIN);
	gpio_put(PICO_SMPS_MODE_PIN, true);
	gpio_init(GPIO_SIO_PIN_01); // Детектор движения
	gpio_set_pulls(GPIO_SIO_PIN_01, false, true);
	gpio_init(GPIO_SDA_I2C0_PIN_4); // I2C0 SDA
	gpio_set_pulls(GPIO_SDA_I2C0_PIN_4, true, false);
	gpio_init(GPIO_SCL_I2C0_PIN_5); // I2C0 SCL
	gpio_set_pulls(GPIO_SCL_I2C0_PIN_5, true, false);
	gpio_init(GPIO_SIO_PIN_15); // Прерывание PN532
	gpio_set_pulls(GPIO_SIO_PIN_15, true, false);
	gpio_init(GPIO_SIO_PIN_08); // Энкодер + PIN 6 + PIN 7
	gpio_set_pulls(GPIO_SIO_PIN_08, true, false);
	gpio_init(GPIO_CLK_PIN_21);
	gpio_set_function(GPIO_CLK_PIN_21, GPIO_FUNC_NULL);
	gpio_set_pulls(GPIO_CLK_PIN_21, false, true);
	gpio_init(GPIO_PWM5_CH_A_PIN_26);
	gpio_set_function(GPIO_PWM5_CH_A_PIN_26, GPIO_FUNC_NULL);
	gpio_set_pulls(GPIO_PWM5_CH_A_PIN_26, false, true);
	gpio_init(GPIO_LED_PIN_25);
	gpio_set_dir(GPIO_LED_PIN_25, GPIO_OUT);
	gpio_init(GPIO_SIO_PIN_09); // Зуммер
	gpio_set_dir(GPIO_SIO_PIN_09, GPIO_OUT);
	gpio_debug_pins_init(true, 1);
	clocks_enable_resus(clock_resus_callback_handler);
	exception_set_exclusive_handler(HARDFAULT_EXCEPTION, hard_fault_callback_handler);
	vreg_set_voltage(VREG_VOLTAGE_1_20);
	bod_set_voltage(BOD_VOLTAGE_0_645);
	gpio_debug_pins_init(true, 2);
	i2c_init(DISPLAY_I2C_BUS, I2C_SPEED_800);
	gpio_init(GPIO_SDA_I2C1_PIN_18);
	gpio_set_pulls(GPIO_SDA_I2C1_PIN_18, true, false);
	gpio_set_function(GPIO_SDA_I2C1_PIN_18, GPIO_FUNC_I2C);
	gpio_init(GPIO_SCL_I2C1_PIN_19);
	gpio_set_pulls(GPIO_SCL_I2C1_PIN_19, true, false);
	gpio_set_function(GPIO_SCL_I2C1_PIN_19, GPIO_FUNC_I2C);
	uint8_t check_power_status = 240;
	bool display_presence = false;
	device_power_status = false;
	display_set_lingth = 0;
	do {
		if (!display_presence)
			display_presence = lcd_display_init();
		sleep_ms(1000);
		device_power_status = usb_device_get_power_config();
	} while (!device_power_status && check_power_status--);
	if (!device_power_status || !display_presence)
		deactivate_device(USER_4_IRQ_IRQ);
	if (watchdog_enable_caused_reboot())
		deactivate_device(USER_1_IRQ_IRQ);
	hot_water_temp = 0xFC90;
	cold_water_temp = 0xFC90;
	display_set_lingth = 240;
	gpio_level_pwm_wrap = 200;
	running_sys_device = true;
	pwm_calculate_freq(GPIO_PWM5_CH_A_PIN_26, 50, 0, true, true, pwm_callback_handler);
	pwm_set_chan_level(PWM_SLICE_5, PWM_CHAN_B, 199);
	i2c_init(i2c0, I2C_SPEED_800);
	gpio_set_function(GPIO_SDA_I2C0_PIN_4, GPIO_FUNC_I2C);
	gpio_set_function(GPIO_SCL_I2C0_PIN_5, GPIO_FUNC_I2C);
	clock_gpio_init(GPIO_CLK_PIN_21, CLOCKS_CLK_GPOUT1_CTRL_AUXSRC_VALUE_CLK_USB, clock_get_hz(clk_usb) / 12000);
	quadrature_encoder_program_init(ENCODER_PIO, ENCODER_PIO_SM, GPIO_SIO_PIN_06, 500);
	gpio_set_irq_enabled_with_callback(GPIO_SIO_PIN_15, GPIO_IRQ_EDGE_FALL, true, gpio_callback_handler);
	irq_set_exclusive_handler(PIO0_IRQ_0, encoder_callback_handler);
	gpio_set_irq_enabled(GPIO_SIO_PIN_08, GPIO_IRQ_EDGE_RISE, true);
	irq_set_enabled(PIO0_IRQ_0_IRQ, true);
	if (!pn532_init_device())
		deactivate_device(USER_2_IRQ_IRQ);
	welcome_procedure_display();
	if (!mifare_card_auth_device(300000, false))
		deactivate_device(USER_0_IRQ_IRQ);
	gpio_set_irq_enabled(GPIO_SIO_PIN_01, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
	rtc_init();
	bme280_init_device();
	before_setup_startup_time();
	nec_receive_program_init(NEC_RECEIVE_PIO, NEC_RECEIVE_PIO_SM, GPIO_SIO_PIN_00);
	irq_set_exclusive_handler(PIO1_IRQ_0_IRQ, receiver_callback_handler);
	irq_set_enabled(PIO1_IRQ_0_IRQ, true);
	add_repeating_timer_us(-1000000 / 2, repeat_timer_callback, NULL, &main_repeating_timer);
	//bme280_read_parameters(&bme280_temperature, &bme280_pressure, &bme280_humidity);
	//irq_set_exclusive_handler(SIO_IRQ_PROC0, core_one_callback_handler);
	//multicore_fifo_clear_irq();
	//irq_set_enabled(SIO_IRQ_PROC0, true);
	//watchdog_enable(7500, true);
	//multicore_launch_core1(core_two_running);
	device_main_run();
}

void before_setup_startup_time(void) {
	datetime_t device_setup_time;
	memcpy(&device_setup_time, &pn532_sector_dat.byte_data[0], sizeof(device_setup_time));
	device_setup_time.sec = 0;
	int64_t old_auth_interval = ((device_setup_time.year - 2023) * 518400) + (device_setup_time.month * 43200) +
			(device_setup_time.day * 1440) + (device_setup_time.hour * 60) + device_setup_time.min;
	if ((old_auth_interval < 399825) || !valid_datetime(&device_setup_time)) {
		device_setup_time.year = 2023;
		device_setup_time.month = 9;
		device_setup_time.day = 7;
		device_setup_time.dotw = 4;
		device_setup_time.hour = 15;
		device_setup_time.min = 45;
	}
	if (setup_current_device_time(&device_setup_time, 0))
		rtc_set_datetime(&device_setup_time);
}




