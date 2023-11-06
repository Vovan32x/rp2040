
/*!
 * File name - header\interrupt.h
 * #include <header\interrupt.h>
 */

#ifndef HEADER_INTERRUPT_H_
#define HEADER_INTERRUPT_H_

#include "hardware/irq.h"

/*!
 *
 */
void core_one_callback_handler(void);

/*!
 *
 */
void pwm_callback_handler(void);

/*!
 *
 */
void encoder_callback_handler(void);

/*!
 *
 */
void receiver_callback_handler(void);

/*!
 *
 */
void hard_fault_callback_handler(void);

/*!
 *
 */
void clock_resus_callback_handler(void);

/*!
 *
 */
void rtc_alarm_callback_handler(void);

/*!
 *
 */
void rtc_overload_callback_handler(void);

#endif
