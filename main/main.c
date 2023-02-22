/*
 * main.c
 *
 *  Created on: 2019. 11. 28.
 *      Author: Owner
 */




#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "mi2c.h"
#include "mp3player.h"
#include "es8388.h"
extern const uint8_t file_start[] asm("_binary_kitosan_mp3_start");
extern const uint8_t file_end[] asm("_binary_kitosan_mp3_end");

void app_main(){
	//이벤트 그룹생성

	es8388_i2c_init();
	i2c_master_init();

	mp3_play_flash(file_start,file_end);
};
