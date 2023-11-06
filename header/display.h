
/*!
 * File name - header\display.h
 * #include <header\display.h>
 */
#ifndef HEADER_DISPLAY_H_
#define HEADER_DISPLAY_H_

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define DISPLAY_I2C_BUS i2c1
#define DISPLAY_DEFAULT_ADDRESS 0x27

/*!
 *
 * @param value
 * @param send_char
 * @return
 */
bool display_send_byte(uint8_t value, bool send_char);

/*!
 *
 * @param blink_cursor
 * @return
 */
bool display_set_blink_cursor(bool blink_cursor);

/*!
 *
 * @return
 */
bool lcd_display_init(void);

/*!
 *
 * @param line
 * @param position
 * @return
 */
bool display_set_cursor(uint8_t line, uint8_t position);

/*!
 *
 * @param string
 */
void display_send_string(const char *string);

/*!
 *
 */
void display_clear(void);

/*!
 *
 */
void welcome_procedure_display(void);

#endif
