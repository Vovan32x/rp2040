
/*!
 * File name - source\interrupt.c
 * #include <header\interrupt.h>
 */

#include <header\main.h>
#include <header\global.h>
#include <header\quadrature.h>
#include <header\nec_receive.h>
#include "hardware\pwm.h"
#include "hardware\pio.h"
#include "hardware\gpio.h"
#include "pico\multicore.h"
#include "hardware/rtc.h"

static int32_t old_encoder_value;

void core_one_callback_handler(void) {
	multicore_fifo_clear_irq();
}

void clock_resus_callback_handler(void) {
	deactivate_device(CLOCKS_IRQ_IRQ);
}

void hard_fault_callback_handler(void) {
	deactivate_device(HARD_FAULT_IRQ);
}

void pwm_callback_handler(void) {
	uint16_t pwm_level = pwm_get_chan_level(PWM_SLICE_5, PWM_CHAN_B);
	if (pwm_level < gpio_level_pwm_wrap) {
		pwm_set_both_levels(PWM_SLICE_5, pwm_get_wrap(PWM_SLICE_5) * (pwm_level + 1) / 1000, pwm_level + 1);
	} else {
		if (pwm_level > gpio_level_pwm_wrap) {
			pwm_set_both_levels(PWM_SLICE_5, pwm_get_wrap(PWM_SLICE_5) * (pwm_level - 1) / 1000, pwm_level - 1);
		} else {
			if (pwm_level == gpio_level_pwm_wrap)
				pwm_set_irq_enabled(PWM_SLICE_5, false);
			if (pwm_level == 199)
				pwm_set_both_levels(PWM_SLICE_5, 0, 199);
		}
	}
	pwm_clear_irq(PWM_SLICE_5);
}

void encoder_callback_handler(void) {
	uint32_t new_value = pio_sm_get(ENCODER_PIO, ENCODER_PIO_SM);
	pio_interrupt_clear(ENCODER_PIO, ENCODER_PIO_SM);
	if (curr_encoder_value == 0) {
		uint16_t temp_proc_value = (new_value - old_encoder_value);
		if (((temp_proc_value / 4) != 0) && ((temp_proc_value % 4) == 0)) {
			old_encoder_value = (int32_t)new_value;
			display_set_lingth = 120;
			curr_encoder_value = (int8_t)(temp_proc_value / 4);
		}
	}
	pio_sm_put(ENCODER_PIO, ENCODER_PIO_SM, 1);
}

void receiver_callback_handler(void) {
	union {
		uint32_t raw_data;
		struct {
			uint8_t address;
			uint8_t inverted_address;
			uint8_t data;
			uint8_t inverted_data;
		};
	} frame;
	frame.raw_data = pio_sm_get(NEC_RECEIVE_PIO, NEC_RECEIVE_PIO_SM);
	pio_interrupt_clear(NEC_RECEIVE_PIO, NEC_RECEIVE_PIO_SM);
	if ((frame.data == (frame.inverted_data ^ 0xFF)) && (frame.address == (frame.inverted_address ^ 0xFF))) {
		nec_received_frame = frame.data;
		infrared_sens_received = true;
	}
}

void rtc_alarm_callback_handler(void) {
	rtc_disable_alarm();
	datetime_t current_alarm_time;
	rtc_get_alarm_time(&current_alarm_time);
	if (rtc_alarm_executed) {
		rtc_alarm_executed = false;
		if (current_alarm_time.dotw == 8)
			current_alarm_time.min -= 1;
		current_alarm_time.dotw = 7;
		current_alarm_time.min -= 2;
		rtc_set_alarm(&current_alarm_time, NULL);

	} else {
		rtc_alarm_executed = true;
		if (current_alarm_time.min == -1) {
			current_alarm_time.min += 1;
			current_alarm_time.dotw = 8;
		}
		current_alarm_time.min += 2;
		rtc_set_alarm(&current_alarm_time, rtc_alarm_callback_handler);
	}
}
