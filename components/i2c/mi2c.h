/*
 * libhj_i2c.h
 *
 *  Created on: 2019. 5. 13.
 *      Author: nayana
 */

#ifndef MAIN_LIBHJ_I2C_H_
#define MAIN_LIBHJ_I2C_H_

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"
#include "esp_log.h"

void i2c_master_init();
esp_err_t i2c_master_write_slave(uint8_t address,uint8_t register_address,uint8_t* data_rd,size_t size);
esp_err_t i2c_master_read_slave(uint8_t address,uint8_t register_address,uint8_t* data_rd,size_t size);
#endif /* MAIN_LIBHJ_I2C_H_ */
