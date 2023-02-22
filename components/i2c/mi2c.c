/*
 * libjh_i2c.c
 *
 *  Created on: 2019. 5. 13.
 *      Author: nayana
 */

#include "mi2c.h"
//static const char *TAG = "I2C1_MASTER";
void i2c_master_init()
{
    int i2c_master_port = I2C_NUM_1;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = 33;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
    conf.scl_io_num = 32;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
    conf.master.clk_speed = 400000;
    i2c_param_config(i2c_master_port, &conf);
    int ret = i2c_driver_install(i2c_master_port,conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {printf("init fail\n");}
}


esp_err_t i2c_master_write_slave(uint8_t address,uint8_t register_address,uint8_t* data_rd,size_t size){
	i2c_cmd_handle_t cmd;
	if(size==0){
		return ESP_OK;
	}
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, address|I2C_MASTER_WRITE, 0x01);
	i2c_master_write_byte(cmd, register_address, 0x01);
	i2c_master_write(cmd,data_rd,size,I2C_MASTER_ACK);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000/portMAX_DELAY);
	i2c_cmd_link_delete(cmd);

	return ret;
 }

esp_err_t i2c_master_read_slave(uint8_t address,uint8_t register_address,uint8_t* data_rd,size_t size){
	i2c_cmd_handle_t cmd;
	if(size==0){
		return ESP_OK;
	}

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, address, 0x01);
	i2c_master_write_byte(cmd, register_address, 0x01);
	i2c_master_stop(cmd);
	//ESP_LOGI(TAG,"s");
	esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000/portMAX_DELAY);
	//ESP_LOGI(TAG,"e");
	i2c_cmd_link_delete(cmd);
	if(ret == ESP_FAIL) return ret;

	cmd=i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, address | I2C_MASTER_READ, 0x01);
	if(size>1){
		i2c_master_read(cmd,data_rd,size-1,I2C_MASTER_ACK);
	}

	i2c_master_read_byte(cmd, data_rd+size-1, I2C_MASTER_NACK);
	i2c_master_stop(cmd);
	//ESP_LOGI(TAG,"ss");
	ret = i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000/portMAX_DELAY);
	//ESP_LOGI(TAG,"ee");
	i2c_cmd_link_delete(cmd);
	return ret;
}
