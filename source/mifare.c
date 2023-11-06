
/*!
 * File name - source\mifare.c
 * #include <header\mifare.h>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <header\mifare.h>
#include <header\display.h>
#include <header\global.h>
#include "hardware/rtc.h"
#include "pico/rand.h"

static uint32_t pn532_uid_number;
static uint16_t pn532_last_code;
static uint8_t auth_key_data[6u];
static bool pn532_answer_ready;

void pn532_execute_interrupt(void) {
	union {
		uint32_t dword_answer[6];
		struct {
			uint16_t word_answer[12];
		};
		struct {
			uint8_t byte_answer[24];
		};
	} answer;
	uint8_t pn532_number_read = (pn532_answer_ready ? 24 : 7);
	if (i2c_read_blocking(PN532_DEFAULT_BUS, PN532_DEFAULT_ADDRESS, &answer.byte_answer[0], pn532_number_read, false) == pn532_number_read) {
		uint8_t checksum_responce = (~answer.byte_answer[5] + 1);
		if ((answer.byte_answer[6] == PN532_TO_HOST_STATUS) && (answer.byte_answer[7] == ((pn532_last_code & 0xFF) + 1)) &&
			(answer.dword_answer[0] == 0xFF000001) && (answer.byte_answer[4] == checksum_responce)) {
			uint16_t temp_value = (answer.byte_answer[7] | (answer.byte_answer[8] << 8));
			switch (temp_value) {
				case 0x0041: {
					if (pn532_last_code == 0x3040)
						memcpy(&pn532_sector_dat.byte_data[0], &answer.byte_answer[9], sizeof(pn532_sector_dat.byte_data));
					pn532_last_code = temp_value;
					break;
				}
				case 0x014B: {
					memcpy(&pn532_uid_number, &answer.byte_answer[14], sizeof(pn532_uid_number));
					pn532_last_code = temp_value;
					break;
				}
				case 0x3203: {
					memcpy(&pn532_sector_dat.byte_data[0], &answer.byte_answer[8], 4);
					pn532_last_code = temp_value;
					break;
				}
				case 0x3F0D: {
					memcpy(&pn532_sector_dat.byte_data[0], &answer.byte_answer[8], 3);
					pn532_last_code = temp_value;
					break;
				}
				case 0xF833:
				case 0x2209:
				case 0x1C0F:
				case 0x1813:
				case 0x1615:
				case 0x0055:
				case 0x0045: {
					pn532_last_code = answer.byte_answer[7];
					break;
				}
				default: {
					pn532_last_code = (answer.byte_answer[7] << 8) | 0xFF;
					break;
				}
			}
		} else {
			pn532_answer_ready = ((answer.dword_answer[0] == 0xFF000001) &&
				(answer.word_answer[2] == 0xFF00) && (pn532_number_read == 7));
		}
	}
}

bool pn532_write_data(uint8_t *write_data, uint8_t data_count) {
	uint8_t buffer[data_count + 8];
	uint8_t offset = 0;
	pn532_answer_ready = false;
	buffer[offset++] = PN532_PREAMBLE_BYTE0;
	buffer[offset++] = PN532_PREAMBLE_BYTE1;
	buffer[offset++] = PN532_PREAMBLE_BYTE2;
	buffer[offset++] = (data_count + 1);
	buffer[offset++] = ~data_count;
	buffer[offset++] = HOST_TO_PN532_STATUS;
	uint8_t checksum = HOST_TO_PN532_STATUS;
	pn532_last_code =  (((write_data[0u] == 0x40) ? write_data[2] : write_data[1]) << 8) | write_data[0u];
	for (uint8_t indx = 0; indx < data_count; indx++) {
		buffer[offset++] = write_data[indx];
		checksum += write_data[indx];
	}
	buffer[offset++] = (~checksum + 1);
	buffer[offset] = PN532_POSTAMBLE_BYTE;
	absolute_time_t transaction_timeout = make_timeout_time_ms(100);
	if (i2c_write_blocking_until(PN532_DEFAULT_BUS, PN532_DEFAULT_ADDRESS, &buffer[0], sizeof(buffer), false, transaction_timeout) == sizeof(buffer)) {
		bool func_time_out = false;
		absolute_time_t start_time = make_timeout_time_ms(250);
		while (!pn532_answer_ready && !func_time_out)
			func_time_out = time_reached(start_time);
		while (((pn532_last_code & 0xFF) != (write_data[0u] + 1)) && !func_time_out && pn532_answer_ready)
			func_time_out = time_reached(start_time);
		return !func_time_out && pn532_answer_ready;
	}
	return false;
}

bool pn5323_auth_sector(uint8_t block, bool b_key) {
	bool auth_result = ((pn532_uid_number != 0) && (block <= 0x3F));
	if (auth_result) {
		uint8_t select_card[2] = { 0x54, 0x01 }; // 0x5401 OK
		pn532_write_data(&select_card[0], sizeof(select_card));
		uint8_t buffer[14] = { 0x40, 0x01, (MIFARE_CMD_AUTH_A + (uint8_t)b_key), block };
		//buffer[2] = (MIFARE_CMD_AUTH_A + (uint8_t)b_key);
		//buffer[3] = block;
		memcpy(&buffer[4], &auth_key_data[0], sizeof(auth_key_data));
		memcpy(&buffer[10], &pn532_uid_number, 4);
		auth_result = pn532_write_data(&buffer[0], sizeof(buffer));
	}
	return auth_result;
}

bool pn532_release_card(bool reinit_module) {
	if (pn532_uid_number != 0) {
		pn532_uid_number = 0;
		uint64_t random_auth_key = get_rand_64();
		memcpy(&auth_key_data[0], &random_auth_key, sizeof(auth_key_data));
		uint8_t release_card[2] = { 0x52, 0x01 }; // 0x5201
		pn532_write_data(&release_card[0u], sizeof(release_card));
	}
	if (reinit_module)
		reinit_module = (pn532_activate() != 0);
	return reinit_module;
}

bool pn532_card_present(bool release) {
	bool present_result = (pn532_uid_number != 0);
	if (present_result) {
		uint8_t select_card[4] = { 0x44, 0x01, 0x54, 0x01 }; // 0x4401 0x5401
		present_result = pn532_write_data(&select_card[0], 2);
		present_result &= pn532_write_data(&select_card[2], 2);
		if (!present_result && release)
			present_result = pn532_release_card(true);
	}
	return present_result;
}

bool pn532_read_block(uint8_t block, bool b_key) {
	if (pn5323_auth_sector(block, b_key)) {
		uint8_t read_block[4] = { 0x40, 0x01, 0x30, block };
		//read_block[3] = block;
		return pn532_write_data(&read_block[0u], sizeof(read_block));
	}
	return false;
}

bool pn532_write_gpio_pin(bool p72_gpio) {
	uint8_t pn532_write_gpio[4] = { 0x08, 0xFF, 0xF7, ((uint)p72_gpio << 2) | 0x01 };
	//pn532_write_gpio[3] = ((uint)p72_gpio << 2) | 0x01;
	return pn532_write_data(&pn532_write_gpio[0], sizeof(pn532_write_gpio));
}

bool pn532_read_gpio_pin(void) {
	uint8_t pn532_read_gpio = 0x0C;
	if (pn532_write_data(&pn532_read_gpio, 1))
		return (pn532_sector_dat.byte_data[1] & 0x02);
	return false;
}

bool pn532_write_block(uint8_t block, bool b_key) {
	bool internal_ph532_write(uint8_t iblock, bool ib_key) {
		if (pn5323_auth_sector(block, b_key)) {
			uint8_t write_block[20] = { 0x40, 0x01, 0xA0, iblock };
			//write_block[3] = iblock;
			uint64_t check_write[2];
			check_write[0] = pn532_sector_dat.qwowd_data[0];
			check_write[1] = pn532_sector_dat.qwowd_data[1];
			memcpy(&write_block[4], &pn532_sector_dat.byte_data[0], sizeof(pn532_sector_dat));
			bool result_write = pn532_write_data(&write_block[0], sizeof(write_block));
			if (result_write) {
				write_block[2] = 0x30;
				result_write = pn532_write_data(&write_block[0], 4);
				result_write &= ((check_write[1] == pn532_sector_dat.qwowd_data[1]) &&
					(check_write[0] == pn532_sector_dat.qwowd_data[0]));
			}
			return result_write;
		}
		return false;
	}
	bool ph532_write_result = false;
	if (block >= 5) {
		if ((block % 4) != 0) {
			ph532_write_result = internal_ph532_write(block, b_key);
		} else {
			uint8_t inv_access_key = pn532_sector_dat.byte_data[6] ^ 0xFF;
			uint8_t rev_access_key = pn532_sector_dat.byte_data[7] ^ 0x0F;
			if (((pn532_sector_dat.byte_data[8] & 0x0F) == (inv_access_key >> 4)) && ((pn532_sector_dat.byte_data[8] >> 4) == (rev_access_key & 0x0F)) &&
				((inv_access_key & 0x0F) == (rev_access_key >> 4))) {
				uint8_t access_block_0 = ((rev_access_key >> 4) & 0x01) | ((pn532_sector_dat.byte_data[8] << 1) & 0x02) | ((pn532_sector_dat.byte_data[8] >> 2) & 0x04);
				uint8_t access_block_1 = ((rev_access_key >> 5) & 0x01) | ((pn532_sector_dat.byte_data[8] << 0) & 0x02) | ((pn532_sector_dat.byte_data[8] >> 3) & 0x04);
				uint8_t access_block_2 = ((rev_access_key >> 6) & 0x01) | ((pn532_sector_dat.byte_data[8] >> 1) & 0x02) | ((pn532_sector_dat.byte_data[8] >> 4) & 0x04);
				uint8_t access_block_3 = ((rev_access_key >> 7) & 0x01) | ((pn532_sector_dat.byte_data[8] >> 2) & 0x02) | ((pn532_sector_dat.byte_data[8] >> 5) & 0x04);
				if (((access_block_3 == 0b00000001) || (access_block_3 == 0b00000011) || (access_block_3 == 0b00000101)) &&
					(access_block_0 != 0b00000111) && (access_block_1 != 0b00000111) && (access_block_2 != 0b00000111)) {
					pn532_sector_dat.byte_data[9] = calc_dallas_crc8(&pn532_sector_dat.byte_data[6], 3);
					ph532_write_result = internal_ph532_write(block, b_key);
				}
			}
		}
	}
	return ph532_write_result;
}

bool pn532_config_access(uint8_t *blok0_access, uint8_t *blok1_access, uint8_t *blok2_access, uint8_t *blok3_access) {
	if (((*blok0_access & 0x0F) != 0b00000111) && ((*blok1_access & 0x0F) != 0b00000111) && ((*blok2_access & 0x0F) != 0b00000111) &&
		(((*blok3_access & 0x0F)== 0b00000001) || ((*blok3_access & 0x0F) == 0b00000011) || ((*blok3_access & 0x0F) == 0b00000101))) {
		uint8_t cfg_bytes_access[3] = { 0 };
		cfg_bytes_access[1] = (*blok0_access & 0x01) | ((*blok1_access << 1) & 0x02) | ((*blok2_access << 2) & 0x04) | ((*blok3_access << 3) & 0x08);
		cfg_bytes_access[0] = ~cfg_bytes_access[1];
		cfg_bytes_access[1] = (cfg_bytes_access[1] << 4);
		cfg_bytes_access[2] = ((*blok0_access >> 1) & 0x01) | (*blok1_access & 0x02) | ((*blok2_access << 1) & 0x04) | ((*blok3_access << 2) & 0x08);
		cfg_bytes_access[0] = cfg_bytes_access[0] | (cfg_bytes_access[2] << 4);
		cfg_bytes_access[2] = ((((*blok0_access >> 2) & 0x01) | ((*blok1_access >> 1) && 0x02) | (*blok2_access & 0x04) | ((*blok3_access << 1) & 0x08)) << 4) | (cfg_bytes_access[2] & 0x0F);
		cfg_bytes_access[1] = (cfg_bytes_access[1] & 0xF0) | ~((cfg_bytes_access[2] >> 4) & 0x0F);
		*blok0_access = cfg_bytes_access[0];
		*blok1_access = cfg_bytes_access[1];
		*blok2_access = cfg_bytes_access[2];
		*blok3_access = calc_dallas_crc8(&cfg_bytes_access[0], 3);
		return true;
	}
	return false;
}

bool pn532_init_device(void) {
	uint8_t set_params[2] = { 0x12, 0x24 };
	uint8_t version_counter = 16;
	uint8_t get_firmware_version = 0x02;
	bool pn532_device_present = false;
	pn532_uid_number = 0;
	pn532_sector_dat.qwowd_data[0] = 0;
	pn532_sector_dat.qwowd_data[1] = 0;
	uint64_t random_auth_key = get_rand_64();
	memcpy(&auth_key_data[0u], &random_auth_key, sizeof(auth_key_data));
	do {
		pn532_device_present = pn532_write_data(&set_params[0], sizeof(set_params));
		if (pn532_device_present) {
			pn532_device_present = pn532_write_data(&get_firmware_version, 1);
			pn532_device_present &= (pn532_sector_dat.dword_data[0] == 0x07060132);
			if (!pn532_device_present)
				sleep_us(250);
		} else {
			sleep_us(250);
		}
	} while (!pn532_device_present && version_counter--);
	if (pn532_device_present) {
		uint8_t cfg_regist[7] = { 0x08, 0xFF, 0xF4, 0xFB, 0xFF, 0xF5, 0xFF };
		if (!pn532_write_data(&cfg_regist[0], sizeof(cfg_regist)))
			return false;
		uint8_t pn532_gpio[4] = { 0x08, 0xFF, 0xF7, 0x01 };
		if (!pn532_write_data(&pn532_gpio[0], sizeof(pn532_gpio)))
			return false;
		uint8_t sam_config[4] = { 0x14, 0x01, 0x02, 0x01 }; // 0x1401
		if (!pn532_write_data(&sam_config[0], sizeof(sam_config)))
			return false;
		uint8_t nxt_rficon[5] = { 0x32, 0x02, 0x00, 0x05, 0x06 }; // 0x3202
		if (!pn532_write_data(&nxt_rficon[0], sizeof(nxt_rficon)))
			return false;
		uint8_t lst_rficon[3] = { 0x32, 0x04, 0xFF }; // 0x0x3204
		if (!pn532_write_data(&lst_rficon[0], sizeof(lst_rficon)))
			return false;
		uint8_t rfi_analog[13] = { 0x32, 0x0A, 0xF8, 0xF4, 0x3F, 0x11, 0x4D, 0x85, 0x61, 0x6F, 0x26, 0x02, 0x87 };
		if (!pn532_write_data(&rfi_analog[0], sizeof(rfi_analog)))
			return false;
	}
	return pn532_device_present;
}

uint8_t pn532_activate(void) {
	uint8_t num_repeat_activate = 4;
	uint8_t pre_rficon[3] = {0x32, 0x01, 0x03 };
	do {
		if (pn532_write_data(&pre_rficon[0], sizeof(pre_rficon))) {
			uint8_t rfi_config[5] = { 0x32, 0x05, 0xFF, 0xFF, 0xFF };
			pn532_write_data(&rfi_config[0], sizeof(rfi_config));
			uint8_t rdy_mifare[3] = { 0x4A, 0x01, 0x00 };
			pn532_write_gpio_pin(true);
			return ((uint)pn532_write_data(&rdy_mifare[0], sizeof(rdy_mifare)) + 1);
		} else {
			sleep_us(150);
		}
	} while (num_repeat_activate--);
	return 0;
}

void pn5532_deactivate(void) {
	pn532_uid_number = 0;
	uint8_t rfi_config[5] = { 0x32, 0x05, 0xFF, 0xFF, 0x00 };
	pn532_write_data(&rfi_config[0], sizeof(rfi_config));
	uint8_t pre_rficon[3] = {0x32, 0x01, 0x02 };
	pn532_write_data(&pre_rficon[0], sizeof(pre_rficon));
	pn532_write_gpio_pin(false);
}

void pn532_sleep_mode(bool deep_sleep) {
	pn532_release_card(false);
	pn5532_deactivate();
	uint8_t set_sleep[3] = { 0x16, ((uint8_t)!deep_sleep << 7), (uint8_t)!deep_sleep };
	//set_sleep[1] &= ((uint8_t)!deep_sleep << 7);
	//set_sleep[2] &= (uint8_t)!deep_sleep;
	pn532_write_data(&set_sleep[0], sizeof(set_sleep));
}

bool mifare_card_auth_device(uint32_t wait_timeout_ms, bool show_last) {
	absolute_time_t internal_proc_time;
	uint8_t display_buffer[17] = { 0 };
	uint8_t temp_auth_key_data[6] = { 0 };
	uint8_t cursor_init_device = 0;
	uint8_t current_hex_value = 0;
	uint8_t number_access_key = 5;
	int8_t apply_encoded_key = 0;
	bool first_last_nibble = true;
	bool first_char_encoded = false;
	bool result_exit_process = false;
	void internal_proc_use_mifare(void) {
		display_set_blink_cursor(false);
		display_clear();
		display_send_string("Use mifare cards");
		display_set_cursor(1, 0);
		display_send_string("to unlock device");
	}
	void internal_proc_first_init(void) {
		display_clear();
		display_set_blink_cursor(true);
		display_send_string("Key ------------");
		display_set_cursor(0, 4);
		cursor_init_device = 0;
		current_hex_value = 0;
		first_last_nibble = true;
		first_char_encoded = false;
		encoder_key_pressed = false;
		internal_proc_time._private_us_since_boot = -1;
		pn532_card_present(true);
	}
	void internal_error_first_init(uint error_number) {
		display_set_blink_cursor(false);
		display_clear();
		number_access_key--;
		display_send_string("Mifare cards not");
		display_set_cursor(1, 0);
		sprintf(&display_buffer[0], "authorized E-#%1X.", error_number);
		display_send_string(&display_buffer[0]);
		sleep_ms(5000);
	}
	if (pn532_activate() >= 1) {
		internal_proc_first_init();
		absolute_time_t wait_execute_time = make_timeout_time_ms(wait_timeout_ms);
		while (!time_reached(wait_execute_time) && device_power_status) {
			if (((cursor_init_device >= 1) && (cursor_init_device <= 12)) || first_char_encoded) {
				if (time_reached(internal_proc_time)) {
					internal_proc_first_init();
				}
			}
			if (curr_encoder_value != 0) {
				if (cursor_init_device <= 11) {
					current_hex_value += curr_encoder_value;
					sprintf(&display_buffer[0], "%1X", current_hex_value & 0x0F);
					display_send_string(&display_buffer[0]);
					display_set_cursor(0, cursor_init_device + 4);
				}
				if (cursor_init_device == 12) {
					display_set_cursor(1, 13);
					apply_encoded_key += curr_encoder_value;
					if (apply_encoded_key <= -1)
						apply_encoded_key = 1;
					else if (apply_encoded_key >= 2)
						apply_encoded_key = 0;
					display_send_string((apply_encoded_key == 1) ? "OK" : "NO");
				}
				internal_proc_time = make_timeout_time_ms(5000);
				first_char_encoded = true;
				curr_encoder_value = 0;
			}
			if (encoder_key_pressed) {
				if (cursor_init_device <= 11) {
					display_send_byte('*', true);
					uint8_t auth_key_position = (cursor_init_device / 2);
					cursor_init_device++;
					if (first_last_nibble)
						temp_auth_key_data[auth_key_position] = current_hex_value << 4;
					else
						temp_auth_key_data[auth_key_position] |= current_hex_value;
					current_hex_value = 0;
					first_last_nibble = !first_last_nibble;
					if (cursor_init_device == 12) {
						display_set_cursor(1, 0);
						display_set_blink_cursor(false);
						display_send_string("Apply key? - NO.");
					}
				} else {
					cursor_init_device++;
					first_char_encoded = false;
					if (apply_encoded_key == 1) {
						pn532_card_present(true);
						if (pn532_uid_number == 0) {
							internal_proc_use_mifare();
							internal_proc_time = make_timeout_time_ms(15000);
							while ((pn532_uid_number == 0) && device_power_status) {
								if (time_reached(internal_proc_time)) {
									internal_error_first_init(1);
									internal_proc_first_init();
									break;
								}
							}
						}
						if (pn532_uid_number != 0) {
#if MIFARE_AUTHORITY_CARD
							bool pn532_emergency_auth = pn532_read_gpio_pin();
							if ((pn532_uid_number == MIFARE_AUTHORITY_CARD) || pn532_emergency_auth) {
#endif
								memcpy(&auth_key_data[0], &temp_auth_key_data[0], sizeof(auth_key_data));
								if (pn532_emergency_auth || pn532_read_block(5, false)) {
									datetime_t last_auth_time;
									memcpy(&last_auth_time, &pn532_sector_dat.byte_data[0], sizeof(last_auth_time));
									int64_t old_auth_interval = ((last_auth_time.year - 2023) * 518400) + (last_auth_time.month * 43200) +
										(last_auth_time.day * 1440) + (last_auth_time.hour * 60) + last_auth_time.min;
									uint16_t modbus_check = (pn532_sector_dat.byte_data[10] << 8) | pn532_sector_dat.byte_data[11];
									pn532_sector_dat.byte_data[10] = calc_dallas_crc8((uint8_t *)&last_auth_time, sizeof(last_auth_time));
									pn532_sector_dat.byte_data[11] = 0;
									for (uint8_t index = 0; index < 7; index++)
										pn532_sector_dat.byte_data[11] += pn532_sector_dat.byte_data[index];
									pn532_sector_dat.byte_data[11] = pn532_sector_dat.byte_data[11] ^ 0x55;
									pn532_sector_dat.byte_data[12] = calc_dallas_crc8(&pn532_sector_dat.byte_data[0], 10);
									uint16_t modbus_crc = calc_crc16_modbus(&pn532_sector_dat.byte_data[0], 9);
									last_auth_time.sec = 0;
									if (!pn532_emergency_auth && ((old_auth_interval < 399825) || (!valid_datetime(&last_auth_time)) ||
											(pn532_sector_dat.byte_data[10] != 0) || (pn532_sector_dat.byte_data[11] != pn532_sector_dat.byte_data[8]) ||
											(pn532_sector_dat.byte_data[12] != 0) || (modbus_check != modbus_crc))) {
										internal_error_first_init(3);
										internal_proc_first_init();
									} else {
										if (show_last) {
											display_set_blink_cursor(false);
											display_clear();
											sprintf(&display_buffer[0], " L/A %02d.%02d.%04d", last_auth_time.day, last_auth_time.month, last_auth_time.year);
											display_send_string(&display_buffer[0]);
											sprintf(&display_buffer[0], "%02d:%02d - OK -", last_auth_time.hour, last_auth_time.min);
											display_set_cursor(1, 2);
											display_send_string(&display_buffer[0]);
											sleep_ms(5000);
										}
										result_exit_process = true;
										break;
									}
								} else {
									internal_error_first_init(5);
									internal_proc_first_init();
								}
#if MIFARE_AUTHORITY_CARD
							} else {
								internal_error_first_init(7);
								internal_proc_use_mifare();
								cursor_init_device = 12;
							}
#endif
						}
					} else {
						internal_proc_first_init();
					}
				}
				internal_proc_time = make_timeout_time_ms(5000);
				encoder_key_pressed = false;
			}
			tight_loop_contents();
		}
		if (!result_exit_process) {
			internal_error_first_init(0);
		}
	} else {
		internal_error_first_init(9);
	}
	pn5532_deactivate();
	curr_encoder_value = 0;
	encoder_key_pressed = false;
	return result_exit_process;
}
