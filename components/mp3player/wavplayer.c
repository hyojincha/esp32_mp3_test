#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/ringbuf.h"
#include "driver/i2s.h"
#include "sdmmc_cmd.h"
#include "../codec/audiocodec.h"
#include "wavplayer.h"

#define BUFF_LEN 64

static const char* TAG="WAV_PLAYER";
static uint8_t *buff;
//static RingbufHandle_t ringbuf;
static int32_t (*callback)(uint8_t *buf, int len);


void wavPlayTask(void* arg){
	buff=(uint8_t*)malloc(BUFF_LEN);

	callback(buff,sizeof(wavheader));
	wavheader* ptr=(wavheader*)buff;
	ESP_LOGI(TAG,"samplate=%d",ptr->samplate);
	ESP_LOGI(TAG,"bitperSample=%d",ptr->bitperSample);
	ESP_LOGI(TAG,"num_of_channel=%d",ptr->num_of_channel);
	ESP_LOGI(TAG,"chunksize=%d",ptr->chunksize);

	size_t totalsize = ptr->chunksize;
	size_t len=0;
	codec_init(ptr->samplate,ptr->bitperSample,ptr->num_of_channel);
	while(totalsize>BUFF_LEN){
		len=callback(buff,BUFF_LEN);
		totalsize-=len;
		i2s_write(0,buff,len,&len,portMAX_DELAY);
	}
	i2s_write(0,buff,totalsize,&len,portMAX_DELAY);
	free(buff);
}

void wav_start(int32_t (*datacallback)(uint8_t *buf, int len)){
	callback=datacallback;

	xTaskCreatePinnedToCore(wavPlayTask, "wavTask", 4096,NULL, tskIDLE_PRIORITY+1, NULL,0) ;
}
