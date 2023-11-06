
/*!
 * File name - source\bme280.c
 * #include <header\bme280.h>
 */

#include "hardware/i2c.h"
#include "header\bme280.h"
#include "header\global.h"

bool bh1750_init_device(void) {
	uint8_t light_transmit = 0b00000001;
	bool bh1750_init_result = (i2c_write_blocking(BH1750_DEFAULT_BUS, BH1750_DEFAULT_ADDRESS, &light_transmit, 1, false) == 1);
	if (bh1750_init_result) {
		light_transmit = 0b00000111;
		bh1750_init_result &= (i2c_write_blocking(BH1750_DEFAULT_BUS, BH1750_DEFAULT_ADDRESS, &light_transmit, 1, false) == 1);
		light_transmit = 0b01000100;
		bh1750_init_result &= (i2c_write_blocking(BH1750_DEFAULT_BUS, BH1750_DEFAULT_ADDRESS, &light_transmit, 1, false) == 1);
		light_transmit = 0b01101010;
		bh1750_init_result &= (i2c_write_blocking(BH1750_DEFAULT_BUS, BH1750_DEFAULT_ADDRESS, &light_transmit, 1, false) == 1);
		light_transmit = 0b00010000;
		bh1750_init_result &= (i2c_write_blocking(BH1750_DEFAULT_BUS, BH1750_DEFAULT_ADDRESS, &light_transmit, 1, false) == 1);
	}
	return bh1750_init_result;
}

bool bme280_init_device(void) {
	uint8_t bme280_transmit[2] = { BME280_RESET_REG, BME280_RESET_VALUE };
	bool bme_init_result = (i2c_write_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, &bme280_transmit[0], 2, false) == 2);
	if (bme_init_result) {
		bme280_transmit[0] = BME280_CONFIG_REG;
		bme280_transmit[1] = CONFIG_STANDBY_500 | CONFIG_FILTER_X16;
		bme_init_result &= (i2c_write_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, &bme280_transmit[0], 2, false) == 2);
		bme280_transmit[0] = CTRL_MEASURE_REG;
		bme280_transmit[1] = CTRL_MEASURE_TEMP_OVX4 | CTRL_MEASURE_PRES_OVX16 | CTRL_MEASURE_NORMAL_MOD;
		bme_init_result &= (i2c_write_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, &bme280_transmit[0], 2, false) == 2);
		bme280_transmit[0] = BME280_CHIP_ID_REG;
		i2c_write_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, &bme280_transmit[0], 1, true);
		bme_init_result &= (i2c_read_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, &bme280_transmit[0], 1, false) == 1);
		if (bme280_transmit[0] == BME280_CHIP_ID_VALUE) {
			bme280_transmit[0] = CTRL_HUMIDITY_REG;
			bme280_transmit[1] = CTRL_HUMIDITY_OVX4;
			bme_init_result &= (i2c_write_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, &bme280_transmit[0], 2, false) == 2);
		}
	}
	return bme_init_result;
}

void bme280_read_parameters(int32_t *temperature, uint32_t *pressure, uint32_t *humidity) {
	bool internal_exit_read_params(bool modify_default) {
		*temperature = (int32_t)((BME280_DEFAULT_TEMPERATURE - (uint)modify_default) * TEMPERATURE_DIVISOR);
		*pressure = (uint32_t)((BME280_DEFAULT_PRESSURE + (uint)modify_default) * PRESSURE_DIVISOR);
		*humidity = (uint32_t)(BME280_DEFAULT_HUMIDITY * HUMIDITY_DIVISOR);
		return bme280_init_device();
	}
	bme280_calib_param_t calib_param = { 0 };
	uint8_t bmx280_params_type = 0;
	if ((*pressure == (uint32_t)(BME280_DEFAULT_PRESSURE * PRESSURE_DIVISOR)) ||
		(*temperature == (int32_t)(BME280_DEFAULT_TEMPERATURE * TEMPERATURE_DIVISOR))) {
		internal_exit_read_params(true);
	} else {
		if (bme280_get_calib_params(&calib_param, &bmx280_params_type)) {
			uint8_t bme280_buffer_read[8] = { 0 };
			uint8_t bme280_read_reg = BME280_STATUS_REG;
			uint8_t count_read_data = 32;
			bool bme280_im_update = true;
			do {
				i2c_write_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, &bme280_read_reg, 1, true);
				i2c_read_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, &bme280_buffer_read[0], 1, false);
				bme280_im_update = (bme280_buffer_read[0] & STATUS_IM_UPDATE);
				if (bme280_im_update)
					sleep_us(25);
			} while (bme280_im_update && count_read_data--);
			if (!bme280_im_update) {
				bme280_read_reg = BME280_PRES_MSB_REG;
				i2c_write_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, &bme280_read_reg, 1, true);
				i2c_read_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, &bme280_buffer_read[0], ((bmx280_params_type == 0x60) ? 8 : 6), false);
				int32_t temp_bme280 = ((bme280_buffer_read[3] << 12) | (bme280_buffer_read[4] << 4) | (bme280_buffer_read[5] >> 4));
				int64_t var_x1 = ((((temp_bme280 >> 3) - ((int32_t)calib_param.dig_T1 << 1))) * ((int32_t)calib_param.dig_T2)) >> 11;
				int64_t var_x2 = (((((temp_bme280 >> 4) - ((int32_t)calib_param.dig_T1)) * ((temp_bme280 >> 4) -
					((int32_t)calib_param.dig_T1))) >> 12) * ((int32_t)calib_param.dig_T3)) >> 14;
				int64_t t_fine = (var_x1 + var_x2);
				*temperature = (int32_t)(((t_fine * (int32_t)5 + (int32_t)128) >> 8));
				temp_bme280 = ((bme280_buffer_read[0] << 12) | (bme280_buffer_read[1] << 4) | (bme280_buffer_read[2] >> 4));
				var_x1 = ((int64_t)t_fine) - 128000;
				var_x2 = var_x1 * var_x1 * (int64_t)calib_param.dig_P6;
				var_x2 = var_x2 + ((var_x1 * (int64_t)calib_param.dig_P5) << 17);
				var_x2 = var_x2 + (((int64_t)calib_param.dig_P4) << 35);
				var_x1 = ((var_x1 * var_x1 * (int64_t)calib_param.dig_P3) >> 8) + ((var_x1 * (int64_t)calib_param.dig_P2) << 12);
				var_x1 = (((((int64_t)1) << 47) + var_x1)) * ((int64_t)calib_param.dig_P1) >> 33;
				if (var_x1 != 0) {
					int64_t pres = (int64_t)1048576 - (int64_t)temp_bme280;
					pres = (((pres << 31) - var_x2) * (int64_t)3125) / var_x1;
					var_x1 = (((int64_t)calib_param.dig_P9) * (pres >> 13) * (pres >> 13)) >> 25;
					var_x2 = (((int64_t)calib_param.dig_P8) * pres) >> 19;
					*pressure = (uint32_t)(((pres + var_x1 + var_x2) >> 8) + (((int64_t)calib_param.dig_P7) << 4));
				} else {
					internal_exit_read_params(true);
				}
				if (bmx280_params_type == BME280_CHIP_ID_VALUE) {
					uint32_t humid_bme280 = ((bme280_buffer_read[6] << 8) | bme280_buffer_read[7]);
					var_x1 = (t_fine - (int32_t)76800);
					var_x1 = (((((humid_bme280 << 14) - (((int32_t)calib_param.dig_H4) << 20) - (((int32_t)calib_param.dig_H5) * var_x1)) +
							((int32_t)16384)) >> 15) * (((((((var_x1 * ((int32_t)calib_param.dig_H6)) >> 10) * (((var_x1 * ((int32_t)calib_param.dig_H3)) >> 11) +
									((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)calib_param.dig_H2) + (int32_t)8192) >> 14));
					var_x1 = (var_x1 - (((((var_x1 >> 15) * (var_x1 >> 15)) >> 7) * ((int32_t)calib_param.dig_H1)) >> 4));
					var_x1 = ((var_x1 < 0) ? 0 : var_x1);
					var_x1 = ((var_x1 > 419430400) ? 419430400 : var_x1);
					*humidity = (uint32_t)(var_x1 >> 12);
				} else {
					internal_exit_read_params(true);
				}
			}
		} else {
			internal_exit_read_params(false);
		}
	}
}

bool bme280_get_calib_params(struct bme280_calib_param *params, uint8_t *bmx_device) {
    uint8_t bme280_calib_buf[25] = { 0 };
    uint8_t bme280_calib_reg = BME280_CHIP_ID_REG;
    i2c_write_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, &bme280_calib_reg, 1, true);
    bool read_calib_result = (i2c_read_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, bmx_device, 1, false) == 1);
    if (read_calib_result && (((*bmx_device >= BMP280_CHIP_MIN_VALUE) && (*bmx_device <= BMP280_CHIP_MAX_VALUE)) || (*bmx_device == BME280_CHIP_ID_VALUE))) {
    	bme280_calib_reg = BME280_CALIB00_REG;
    	uint8_t reg_read_count = ((*bmx_device == BME280_CHIP_ID_VALUE) ? 25 : 24);
    	i2c_write_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, &bme280_calib_reg, 1, true);
    	read_calib_result = (i2c_read_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, &bme280_calib_buf[0], reg_read_count, false) == reg_read_count);
    	if (read_calib_result) {
    		params->dig_T1 = (uint16_t)((bme280_calib_buf[1] << 8) | bme280_calib_buf[0]);
    		params->dig_T2 = (int16_t)((bme280_calib_buf[3] << 8) | bme280_calib_buf[2]);
    		params->dig_T3 = (int16_t)((bme280_calib_buf[5] << 8) | bme280_calib_buf[4]);
    		params->dig_P1 = (uint16_t)((bme280_calib_buf[7] << 8) | bme280_calib_buf[6]);
    		params->dig_P2 = (int16_t)((bme280_calib_buf[9] << 8) | bme280_calib_buf[8]);
    		params->dig_P3 = (int16_t)((bme280_calib_buf[11] << 8) | bme280_calib_buf[10]);
    		params->dig_P4 = (int16_t)((bme280_calib_buf[13] << 8) | bme280_calib_buf[12]);
    		params->dig_P5 = (int16_t)((bme280_calib_buf[15] << 8) | bme280_calib_buf[14]);
    		params->dig_P6 = (int16_t)((bme280_calib_buf[17] << 8) | bme280_calib_buf[16]);
    		params->dig_P7 = (int16_t)((bme280_calib_buf[19] << 8) | bme280_calib_buf[18]);
    		params->dig_P8 = (int16_t)((bme280_calib_buf[21] << 8) | bme280_calib_buf[20]);
    		params->dig_P9 = (int16_t)((bme280_calib_buf[23] << 8) | bme280_calib_buf[22]);
    		if (*bmx_device == 0x60) {
    			bme280_calib_reg = BME280_CALIB26_REG;
    			params->dig_H1 = (uint8_t)(bme280_calib_buf[25]);
    			i2c_write_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, &bme280_calib_reg, 1, true);
    			read_calib_result = (i2c_read_blocking(BME280_DEFAULT_BUS, BME280_DEFAULT_ADDRESS, &bme280_calib_buf[0], 8, false) == 8);
    			if (read_calib_result) {
    				params->dig_H2 = (int16_t)((bme280_calib_buf[1] << 8) | bme280_calib_buf[0]);
    				params->dig_H3 = (int8_t)(bme280_calib_buf[2]);
    				params->dig_H4 = (int16_t)((bme280_calib_buf[4] & 0x0F) | (bme280_calib_buf[3] << 4));
    				params->dig_H5 = (int16_t)((bme280_calib_buf[5] >> 4) | (bme280_calib_buf[6] << 4));
    				params->dig_H6 = (int8_t)(bme280_calib_buf[7]);
    			}
    		}
    	}
    }
    return read_calib_result;
}
