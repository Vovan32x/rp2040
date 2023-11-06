
/*!
 * File name - header\bme280.h
 * #include <header\bme280.h>
 */

#ifndef HEADER_BME280_H_
#define HEADER_BME280_H_

#include "header\global.h"

#define BME280_DEFAULT_BUS i2c0
#define BME280_DEFAULT_ADDRESS 0x76

#define BH1750_DEFAULT_BUS i2c0
#define BH1750_DEFAULT_ADDRESS 0x23

#define BME280_RESET_REG 0xE0
#define BME280_RESET_VALUE 0xB6

#define BME280_STATUS_REG 0xF3
#define STATUS_MEASURING 0b00001000
#define STATUS_IM_UPDATE 0b00000001

#define BME280_PRES_MSB_REG 0xF7
#define BME280_CHIP_ID_REG 0xD0
#define BME280_CALIB00_REG 0x88
#define BME280_CALIB26_REG 0xE1
#define BME280_CHIP_ID_VALUE 0x60
#define BMP280_CHIP_MIN_VALUE 0x56
#define BMP280_CHIP_MAX_VALUE 0x58

#define BME280_CONFIG_REG 0xF5
#define CONFIG_STANDBY_0_0 (0b000 << 5)
#define CONFIG_STANDBY_62_5 (0b001 << 5)
#define CONFIG_STANDBY_125 (0b010 << 5)
#define CONFIG_STANDBY_250 (0b011 << 5)
#define CONFIG_STANDBY_500 (0b100 << 5)
#define CONFIG_STANDBY_1K (0b101 << 5)
#define CONFIG_STANDBY_10S (0b110 << 5)
#define CONFIG_STANDBY_20S (0b111 << 5)
#define CONFIG_FILTER_OFF (0b000 << 2)
#define CONFIG_FILTER_X2 (0b001 << 2)
#define CONFIG_FILTER_X4 (0b010 << 2)
#define CONFIG_FILTER_X8 (0b011 << 2)
#define CONFIG_FILTER_X16 (0b100 << 2)

#define CTRL_MEASURE_REG 0xF4
#define CTRL_MEASURE_TEMP_SKIP (0b000 << 5)
#define CTRL_MEASURE_TEMP_OVX1 (0b001 << 5)
#define CTRL_MEASURE_TEMP_OVX2 (0b010 << 5)
#define CTRL_MEASURE_TEMP_OVX4 (0b011 << 5)
#define CTRL_MEASURE_TEMP_OVX8 (0b100 << 5)
#define CTRL_MEASURE_TEMP_OVX16 (0b101 << 5)
#define CTRL_MEASURE_PRES_SKIP (0b000 << 2)
#define CTRL_MEASURE_PRES_OVX1 (0b001 << 2)
#define CTRL_MEASURE_PRES_OVX2 (0b010 << 2)
#define CTRL_MEASURE_PRES_OVX4 (0b011 << 2)
#define CTRL_MEASURE_PRES_OVX8 (0b100 << 2)
#define CTRL_MEASURE_PRES_OVX16 (0b101 << 2)
#define CTRL_MEASURE_SLLEP_MOD 0b00
#define CTRL_MEASURE_FORCE_MOD 0b01
#define CTRL_MEASURE_NORMAL_MOD 0b11

#define CTRL_HUMIDITY_REG 0xF2
#define CTRL_HUMIDITY_SKIP 0b000
#define CTRL_HUMIDITY_OVX1 0b001
#define CTRL_HUMIDITY_OVX2 0b010
#define CTRL_HUMIDITY_OVX4 0b011
#define CTRL_HUMIDITY_OVX8 0b100
#define CTRL_HUMIDITY_OVX16 0b101

#define BME280_DEFAULT_TEMPERATURE -45.0f // -45.0
#define BME280_DEFAULT_PRESSURE 950.0f // 950`.0
#define BME280_DEFAULT_HUMIDITY 120.0f // `120.0
#define PRESSURE_DIVISOR_MMH 133.322368f
#define PRESSURE_DIVISOR_HPA 256.0f
#define PRESSURE_DIVISOR (PRESSURE_DIVISOR_MMH * PRESSURE_DIVISOR_HPA)
#define TEMPERATURE_DIVISOR 100.0f
#define HUMIDITY_DIVISOR 1024.0f
#define FARENGEIT_ADD_VALUE 32.0f
#define FARENGEIT_MULTIPLIER 1.8f

/*! Структура для калибровочных констант устройства BME/BMP280
 *
 */
typedef struct bme280_calib_param {
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
} bme280_calib_param_t;

/*! Процедура инициализации BH1750
 *
 * @return true если устройство BH1750 обнаружено и успешно инициализировано
 */
bool bh1750_init_device(void);

/*! Процедура инициализации BME280
 *
 * @return true если устройство BME/BMP280 обнаружено и успешно инициализировано
 */
bool bme280_init_device(void);

/*!
 *
 * @param params ссылка на структуру для хранения калибровочных констант устройства
 * @param bmx_device ссылка на 8-ми битную переменную хранения идентификатора устройства
 * @return true если калибровочные константы и идентификатор были успешно прочитаны
 */
bool bme280_get_calib_params(struct bme280_calib_param *params, uint8_t *bmx_device);

/*!
 *
 * @param temperature ссылка на 32-х битную переменную хранения температуры воздуха
 * @param pressure ссылка на 32-х битную переменную хранения атмосферного давления
 * @param humidity ссылка на 32-х битную переменную хранения влажности воздуха
 */
void bme280_read_parameters(int32_t *temperature, uint32_t *pressure, uint32_t *humidity);

#endif
