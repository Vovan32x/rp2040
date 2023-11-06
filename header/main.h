
/*!
 * File name - header\main.h
 * #include <header\main.h>
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"

/*!
 *
 * @param gpio
 * @param event_mask
 */
void gpio_callback_handler(uint gpio, uint32_t event_mask);


/*!
 *
 * @return
 */
int main(void);

/*!
 *
 * @param exception_num
 */
void deactivate_device(int exception_num);

/*!
 *
 * @param setup_time
 * @param wait_setup_ms
 * @return
 */
bool setup_current_time(datetime_t *setup_time, uint32_t wait_setup_ms);

/*!
 *
 */
void on_encoder_left_main_run(void);

/*!
 *
 */
void on_encoder_right_main_run(void);

/*!
 *
 */
void on_encoder_keypre_main_run(void);

/*!
 *
 */
void before_setup_startup_time(void);

#endif /* MAIN_H_ */
