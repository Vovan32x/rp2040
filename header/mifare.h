
/*!
 * File name - header\mifare.h
 * #include <header\mifare.h>
 */

#ifndef HEADER_MIFARE_H_
#define HEADER_MIFARE_H_

#include "header\global.h"

#define PN532_DEFAULT_BUS i2c0
#define PN532_DEFAULT_ADDRESS 0x24
#define MIFARE_AUTHORITY_CARD 0xF68C7663

#define PN532_TO_HOST_STATUS 0xD5
#define HOST_TO_PN532_STATUS 0xD4
#define PN532_GET_FIRMWARE_VER 0x02
#define PN532_PREAMBLE_BYTE0 0x00
#define PN532_PREAMBLE_BYTE1 0x00
#define PN532_PREAMBLE_BYTE2 0xFF
#define PN532_POSTAMBLE_BYTE 0x00

#define MIFARE_CMD_AUTH_A 0x60
#define MIFARE_CMD_AUTH_B 0x61
#define MIFARE_CMD_DECREMENT 0xC0
#define MIFARE_CMD_INCREMENT 0xC1
#define MIFARE_CMD_RESTORE 0xC2
#define MIFARE_CMD_TRANSFER 0xB0

/*!
 *
 */
void pn532_execute_interrupt(void);

/*!
 *
 * @param write
 * @param count
 * @return
 */
bool pn532_write_data(uint8_t *write, uint8_t count);

/*!
 *
 * @param block
 * @param b_key
 * @return
 */
bool pn5323_auth_sector(uint8_t block, bool b_key);

/*!
 *
 * @param reinit_module
 * @return
 */
bool pn532_release_card(bool reinit_module);

/*!
 *
 * @param release
 * @return
 */
bool pn532_card_present(bool release);

/*!
 *
 * @param block
 * @param b_key
 * @return
 */
bool pn532_read_block(uint8_t block, bool b_key);

/*!
 *
 * @param block
 * @param b_key
 * @return
 */
bool pn532_write_block(uint8_t block, bool b_key);

/*!
 *
 * @return
 */
bool pn532_init_device(void);

/*!
 *
 * @return
 */
uint8_t pn532_activate(void);

/*!
 *
 */
void pn5532_deactivate(void);

/*!
 *
 * @param deep_sleep
 */
void pn532_sleep_mode(bool deep_sleep);

/*!
 *
 * @return
 */
bool pn532_read_gpio_pin(void);

/*!
 *
 * @param p72_gpio
 * @return
 */
bool pn532_write_gpio_pin(bool p72_gpio);

/*!
 *
 * @param wait_timeout_ms
 * @param show_last
 * @return
 */
bool mifare_card_auth_device(uint32_t wait_timeout_ms, bool show_last);

/*! Генерация правильной комбинации для бит доступа к секторам
 *
 * @param blok0_access 4-х битное значение для доступа к 0 блоку (data)
 * @param blok1_access 4-х битное значение для доступа к 1 блоку (data)
 * @param blok2_access 4-х битное значение для доступа к 2 блоку (data)
 * @param blok3_access 4-х битное значение для доступа к 3 блоку (trailer)
 * @return true если значение бит доступа разрешает чтение данных всех блоков по ключу А или В
 * Значение blok0_access, blok1_access, blok2_access перезаписывается правильным значением для
 * возможности непосредственной записи в 3 блок (trailer). Значение blk3_access перезаписывается
 * контрольной суммой Dallas CRC-8 для возможности проверки целостности данных для бит доступа.
 */
bool pn532_config_access(uint8_t *blok0_access, uint8_t *blok1_access, uint8_t *blok2_access, uint8_t *blok3_access);

/*!
 *
 * @return
 */
bool pn532_access_blocks(void);

#endif
