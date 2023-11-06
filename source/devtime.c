
/*!
 * File name - source\devtime.c
 * #include <header\devtime.h>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <header\global.h>
#include <header\display.h>
#include "hardware/rtc.h"
#include <header\devtime.h>
#include <header\interrupt.h>

void display_send_alarm_time(void) {
	void internal_disp_send_alarm_time(uint8_t position, int8_t convert_value) {
		uint8_t display_buffer[4];
		display_set_cursor(1, position);
		sprintf(&display_buffer[0], "%02d", convert_value);
		display_send_string(&display_buffer[0]);
	}
	datetime_t current_dev_time;
	int8_t temporary_day = -1;
	int8_t temporary_hour = -1;
	int8_t temporary_min = -1;
	int8_t temporary_sec = -1;
	bool temp_light_display = true;
	bool display_divisor = true;
	encoder_key_pressed = false;
	display_clear();
	display_set_cursor(1, 0);
	display_send_byte(':', true);
	display_set_cursor(1, 0);
	display_send_byte(':', true);
	display_set_cursor(1, 11);
	display_send_string("CLR");
	main_repeating_timer.delay_us = -1000000 / 4;
	do {
		rtc_get_datetime(&current_dev_time);
		if (temporary_day != current_dev_time.day) {
			temporary_day = current_dev_time.day;
			display_send_date(7, current_dev_time.day, current_dev_time.month, current_dev_time.year);
		}
		if (temporary_hour != current_dev_time.hour) {
			temporary_hour = current_dev_time.hour;
			internal_disp_send_alarm_time(2, current_dev_time.hour);
		}
		if (temporary_min != current_dev_time.min) {
			temporary_min = current_dev_time.min;
			internal_disp_send_alarm_time(4, current_dev_time.min);
			temp_light_display = true;
		}
		if (temp_light_display) {
			display_set_lingth = 120;
			internal_disp_send_alarm_time(7, current_dev_time.sec);
		} else {
			display_set_lingth = 0;
			display_set_cursor(1, 4);
		}
		gpio_put(GPIO_LED_PIN_25, temp_light_display);
		temp_light_display = !temp_light_display;
		while (!repeat_timer_execute) {
			if (encoder_key_pressed) {
				datetime_t current_alarm_time;
				rtc_get_alarm_time(&current_alarm_time);
				if (rtc_alarm_executed) {
					if (current_alarm_time.dotw == 8)
						current_alarm_time.min -= 1;
					current_alarm_time.dotw = 7;
					current_alarm_time.min -= 2;
					rtc_set_alarm(&current_alarm_time, NULL);
					rtc_alarm_executed = false;
				}
				encoder_key_pressed = false;
			}
		}
		repeat_timer_execute = false;
	} while (rtc_alarm_executed && device_power_status);
	display_set_lingth = 120;
	main_repeating_timer.delay_us = -1000000 / 2;
	gpio_put(GPIO_LED_PIN_25, false);
	on_exit_to_main_cycle();
	curr_encoder_value = 0;
}

void display_send_date(uint8_t send_dotw, int8_t send_day, int8_t send_month, int16_t send_year) {
	uint8_t send_data_buff[17] = { 0 };
	display_set_cursor(0, 0);
	char *day_of_weak[8] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "ALR" };
	uint8_t buff_position = sprintf(&send_data_buff[0], " %s ", day_of_weak[send_dotw]);
	if (send_day >= 1)
		buff_position += sprintf(&send_data_buff[buff_position], "%02d.", send_day);
	else
		buff_position += sprintf(&send_data_buff[buff_position], "%s.", "**");
	if (send_month >= 1)
		buff_position += sprintf(&send_data_buff[buff_position], "%02d.", send_month);
	else
		buff_position += sprintf(&send_data_buff[buff_position], "%s.", "**");
	if (send_year >= 1)
		sprintf(&send_data_buff[buff_position], "%04d", send_year);
	else
		sprintf(&send_data_buff[buff_position], "%s", "****");
	display_send_string(&send_data_buff[0]);
}

void update_current_time_set(datetime_t *setup_time, uint8_t position, uint8_t run_set_opt, bool before_setup) {
	uint8_t cursor_on_display[5] = { 6, 9, 14, 67, 70 };
	uint8_t display_message[17] = { 0 };
	if ((position <= 2) || before_setup) {
		display_send_date(setup_time->dotw, setup_time->day, setup_time->month, setup_time->year);
	}
	if ((position >= 3) || before_setup) {
		display_set_cursor(1, 2);
		uint8_t buff_position = 0;
		if (setup_time->hour >= 0)
			buff_position = sprintf(&display_message[0], "%02d:", setup_time->hour);
		else
			buff_position = sprintf(&display_message[0], "%s:", "**");
		if (setup_time->min >= 0)
			sprintf(&display_message[buff_position], "%02d:", setup_time->min);
		else
			sprintf(&display_message[buff_position], "%s:", "**");
		display_send_string(&display_message[0]);
		if (before_setup)
			display_send_string("00");
		display_set_cursor(1, 11);
		char *alarm_options[4] = { "RUN", "SET", "OUT", "CLR" };
		display_send_string(alarm_options[run_set_opt]);
	}
	if (position <= 4) {
		display_set_cursor(0, cursor_on_display[position]);
		display_set_blink_cursor(true);
	} else {
		display_set_blink_cursor(false);
	}
}

void common_proc_setup_time(datetime_t *time_setup, uint8_t *switch_pos, bool current_time) {
	uint8_t days_of_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	bool leaped_setup_year = !((time_setup->year % 4) || (((time_setup->year % 100) == 0) &&
		(time_setup->year % 400))) && (time_setup->month == 2);
	switch (*switch_pos) {
		case 0: {
			time_setup->day += curr_encoder_value;
			if (time_setup->day <= 0) {
				if (current_time || ((time_setup->year <= 0) && (time_setup->hour <= -1) && (time_setup->month <= 0) && (time_setup->min <= -1)))
					time_setup->day = days_of_month[time_setup->month - 1] + (int8_t)leaped_setup_year;
				else {
					if (time_setup->day == -1)
						time_setup->day = 31;
				}
			}
			if ((time_setup->day >= 32) || (time_setup->day >= days_of_month[time_setup->month - 1] + 1 + (int8_t)leaped_setup_year)) {
				if (current_time || ((time_setup->year <= 0) && (time_setup->hour <= -1) && (time_setup->month <= 0) && (time_setup->min <= -1)))
					time_setup->day = 1;
				else {
					time_setup->day = 0;
				}
			}
			if (current_time)
				time_setup->dotw = rtc_calc_dotw(time_setup);
			break;
		}
		case 1: {
			time_setup->month += curr_encoder_value;
			if (time_setup->month <= 0) {
				if (current_time || ((time_setup->year <= 0) && (time_setup->hour <= -1) && (time_setup->day <= 0) && (time_setup->min <= -1)))
					time_setup->month = 12;
				else {
					if (time_setup->month == -1)
						time_setup->month = 12;
				}
			}
			if (time_setup->month >= 13) {
				if (current_time || ((time_setup->year <= 0) && (time_setup->hour <= -1) && (time_setup->day <= 0) && (time_setup->min <= -1)))
					time_setup->month = 1;
				else
					time_setup->month = 0;
			}
			if (current_time) {
				while (time_setup->day > days_of_month[time_setup->month - 1] + (uint8_t)leaped_setup_year)
					time_setup->day--;
				time_setup->dotw = rtc_calc_dotw(time_setup);
			}
			break;
		}
		case 2: {
			time_setup->year += curr_encoder_value;
			bool leaped_setup_year = !((time_setup->year % 4) || (((time_setup->year % 100) == 0) &&
					(time_setup->year % 400))) && (time_setup->month == 2);
			if (current_time || ((time_setup->hour <= 0) && (time_setup->month <= 0) && (time_setup->day <= 0) && (time_setup->min <= -1))) {
				if (time_setup->year <= 1899)
					time_setup->year = 4023;
				if (time_setup->year >= 4024)
					time_setup->year = 1900;
			} else {
				if (time_setup->year == 1899) {
					time_setup->year = -1;
				} else {
					if (time_setup->year == -2) {
						time_setup->year = 4023;
					} else {
						if (time_setup->year == 0) {
							time_setup->year = 1900;
						} else {
							if (time_setup->year == 4024)
								time_setup->year = -1;
						}
					}
				}
			}
			if (current_time) {
				while (time_setup->day > days_of_month[time_setup->month - 1] + (uint8_t)leaped_setup_year)
					time_setup->day--;
				time_setup->dotw = rtc_calc_dotw(time_setup);
			}
			break;
		}
		case 3: {
			time_setup->hour += curr_encoder_value;
			if (time_setup->hour <= -1) {
				if (current_time || ((time_setup->year <= 0) && (time_setup->month <= 0) && (time_setup->day <= 0) && (time_setup->min <= -1)))
					time_setup->hour = 23;
				else {
					if (time_setup->hour == -2)
						time_setup->hour = 23;
				}
			}
			if (time_setup->hour >= 24) {
				if (current_time || ((time_setup->year <= 0) && (time_setup->month <= 0) && (time_setup->day <= 0) && (time_setup->min <= -1)))
					time_setup->hour = 0;
				else
					time_setup->hour = -1;
			}
			break;
		}
		case 4: {
			time_setup->min += curr_encoder_value;
			if (time_setup->min <= -1) {
				if (current_time || ((time_setup->year <= 0) && (time_setup->month <= 0) && (time_setup->day <= 0) && (time_setup->hour <= -1)))
					time_setup->min = 59;
				else {
					if (time_setup->min == -2)
						time_setup->min = 59;
				}
			}
			if (time_setup->min >= 60) {
				if (current_time || ((time_setup->year <= 0) && (time_setup->month <= 0) && (time_setup->day <= 0) && (time_setup->hour <= -1)))
					time_setup->min = 0;
				else {
					time_setup->min = -1;
				}
			}
			break;
		}
		case 5: {
			break;
		}
		default: {
			*switch_pos = 0;
			break;
		}
	}
}

void setup_alarm_device_time(void) {
	datetime_t alarm_setup_time;
	uint16_t alarm_match_flags = rtc_get_alarm_time(&alarm_setup_time);
	uint8_t cursor_on_display[5] = { 6, 9, 14, 67, 70 };
	uint8_t display_msg[17] = { 0 };
	uint8_t cursor_time_position = 0;
	int8_t temp_setup_second = 0;
	bool alarm_match_enabled = ((alarm_match_flags & RTC_IRQ_SETUP_0_MATCH_ENA_MASK) != 0);
	int8_t run_set_opt_values = (1 + (uint)alarm_match_enabled);
	display_clear();
	alarm_setup_time.sec = 0;
	alarm_setup_time.dotw = 7;
	curr_encoder_value = 0;
	absolute_time_t wait_setup_time = make_timeout_time_ms(30000);
	update_current_time_set(&alarm_setup_time, cursor_time_position, run_set_opt_values, true);
	while (!time_reached(wait_setup_time)) {
		datetime_t temp_setup_time;
		rtc_get_datetime(&temp_setup_time);
		if (temp_setup_second != temp_setup_time.sec) {
			temp_setup_second = temp_setup_time.sec;
			display_set_cursor(1, 8);
			sprintf(&display_msg[0], "%02d", temp_setup_time.sec);
			display_send_string(&display_msg[0]);
			display_set_cursor(0, cursor_on_display[cursor_time_position]);
		}
		if (curr_encoder_value != 0) {
			wait_setup_time = make_timeout_time_ms(30000);
			common_proc_setup_time(&alarm_setup_time, &cursor_time_position, false);
			if (cursor_time_position == 5) {
				run_set_opt_values += curr_encoder_value;
				if (run_set_opt_values < 0)
					run_set_opt_values = (2 + (uint)alarm_match_enabled);
				else if (run_set_opt_values >= (3 + (uint)alarm_match_enabled))
					run_set_opt_values = 0;
			}
			update_current_time_set(&alarm_setup_time, cursor_time_position, run_set_opt_values, false);
			curr_encoder_value = 0;
		}
		if (encoder_key_pressed) {
			wait_setup_time = make_timeout_time_ms(30000);
			if (cursor_time_position >= 5) {
				if (run_set_opt_values == 0) {
					int64_t check_current_time = 0;
					int64_t check_alarm_time = 1;
					if ((alarm_setup_time.year >= 1) && (alarm_setup_time.month >= 1) && (alarm_setup_time.day >= 1) &&
						(alarm_setup_time.hour >= 0) && (alarm_setup_time.min >= 0)) {
						check_current_time = ((temp_setup_time.year - 2023) * 518400) + (temp_setup_time.month * 43200) +
							(temp_setup_time.day * 86400) + (temp_setup_time.hour * 60) + temp_setup_time.min + 1;
						check_alarm_time = ((alarm_setup_time.year <= 0) ? ((temp_setup_time.year - 2023) * 518400) : ((alarm_setup_time.year - 2023) * 518400)) +
							((alarm_setup_time.month <= 0) ? (temp_setup_time.month * 43200) : (alarm_setup_time.month * 43200)) +
							((alarm_setup_time.day <= 0) ? (temp_setup_time.day * 86400) : (alarm_setup_time.day * 86400)) +
							((alarm_setup_time.hour <= -1) ? (temp_setup_time.hour * 60) : (alarm_setup_time.hour * 60)) +
							((alarm_setup_time.min <= -1) ? temp_setup_time.min : alarm_setup_time.min);
					}
					if (check_current_time >= check_alarm_time) {
						cursor_time_position = 0;
						curr_encoder_value = 0;
						run_set_opt_values = (1 + (uint)alarm_match_enabled);
						alarm_match_flags = rtc_get_alarm_time(&alarm_setup_time);
						alarm_match_enabled = ((alarm_match_flags & RTC_IRQ_SETUP_0_MATCH_ENA_MASK) != 0);
						update_current_time_set(&alarm_setup_time, cursor_time_position, run_set_opt_values, true);
						encoder_key_pressed = false;
						break;
					}
					alarm_setup_time.sec = 0;
					rtc_set_alarm(&alarm_setup_time, rtc_alarm_callback_handler);
					encoder_key_pressed = false;
					break;
				} else {
					if (run_set_opt_values == 2) {
						encoder_key_pressed = false;
						break;
					} else {
						if (run_set_opt_values == 3) {
							rtc_set_alarm(&alarm_setup_time, NULL);
							encoder_key_pressed = false;
							break;
						} else {
							cursor_time_position = -1;
						}
					}
				}
			}
			cursor_time_position += 1;
			update_current_time_set(&alarm_setup_time, cursor_time_position, run_set_opt_values, false);
			encoder_key_pressed = false;
		}
	}
	on_exit_to_main_cycle();
}

bool setup_current_device_time(datetime_t *setup_time, uint32_t wait_setup_ms) {
	uint8_t cursor_on_display[5] = { 6, 9, 14, 67, 70 };
	uint8_t display_msg[17] = { 0 };
	uint8_t cursor_time_position = 0;
	int8_t temp_setup_second = 0;
	bool run_proc_setup_time = true;
	bool run_set_opt_value = true;
	bool setup_time_result = false;
	display_clear();
	setup_time->sec = 0;
	rtc_set_datetime(setup_time);
	absolute_time_t wait_setup_time = make_timeout_time_ms(wait_setup_ms);
	update_current_time_set(setup_time, cursor_time_position, (uint8_t)run_set_opt_value, true);
	while (run_proc_setup_time) {
		if (wait_setup_ms > 0)
			run_proc_setup_time = !time_reached(wait_setup_time);
		datetime_t temp_setup_time;
		rtc_get_datetime(&temp_setup_time);
		if (temp_setup_second != temp_setup_time.sec) {
			temp_setup_second = temp_setup_time.sec;
			display_set_cursor(1, 8);
			sprintf(&display_msg[0], "%02d", temp_setup_time.sec);
			display_send_string(&display_msg[0]);
			display_set_cursor(0, cursor_on_display[cursor_time_position]);
		}
		if (curr_encoder_value != 0) {
			if (wait_setup_ms > 0)
				wait_setup_time = make_timeout_time_ms(wait_setup_ms);
			if (cursor_time_position == 5)
				run_set_opt_value = !run_set_opt_value;
			else
				common_proc_setup_time(setup_time, &cursor_time_position, true);
			update_current_time_set(setup_time, cursor_time_position, (uint8_t)run_set_opt_value, false);
			curr_encoder_value = 0;
		}
		if (encoder_key_pressed) {
			if (wait_setup_ms > 0)
				wait_setup_time = make_timeout_time_ms(wait_setup_ms);
			if (cursor_time_position == 5) {
				if (!run_set_opt_value) {
					setup_time->sec = 0;
					setup_time_result = true;
					run_proc_setup_time = false;
				} else {
					cursor_time_position = -1;
				}
			}
			cursor_time_position += 1;
			if (run_proc_setup_time)
				update_current_time_set(setup_time, cursor_time_position, (uint8_t)run_set_opt_value, false);
			encoder_key_pressed = false;
		}
	}
	return setup_time_result;
}
