
/*!
 * File nam, - header\devtime.h
 * #include <header\devtime.h>
 */
#ifndef HEADER_DEVTIME_H_
#define HEADER_DEVTIME_H_

#include "pico/stdlib.h"

void display_send_alarm_time(void);

/*!
 *
 */
void setup_alarm_device_time(void);

/*!
 *
 * @param setup_time
 * @param wait_setup_ms
 * @return
 */
bool setup_current_device_time(datetime_t *setup_time, uint32_t wait_setup_ms);

/*!
 *
 * @param send_dotw
 * @param send_day
 * @param send_month
 * @param send_year
 */
void display_send_date(uint8_t send_dotw, int8_t send_day, int8_t send_month, int16_t send_year);

#endif /* HEADER_DEVTIME_H_ */
