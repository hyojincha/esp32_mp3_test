/*
 * mp3player.c
 *
 *  Created on: 2020. 6. 2.
 *      Author: Owner
 */

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
#include "../mp3_decoder/mp3common.h"
#include "../codec/audiocodec.h"
#include "mp3player.h"

#define IN_BUFF_SIZE 2*1024
#define USE_PSRAM	0

static uint8_t *mp3buff=NULL;
static int16_t *pcmbuffer=NULL;
static size_t pcmsize=0;
static HMP3Decoder hMP3Decoder;
static int32_t (*callback)(uint8_t *buf, int len);

static TaskHandle_t decode_handle;
static TaskHandle_t i2s_handle;

static const char* TAG="MP3_PLAYER";

#define MP3_CALLBACK_END		0
#define MP3_CALLBACK_DATAEND	-20
#define MP3_CALLBACK_REPEAT		-10

static mp3_status_t MP3_STATUS=MP3_STOP;
static uint8_t *mp3_inbuffer=NULL;
static uint8_t *playbuffer=NULL;

static RingbufHandle_t ringbuf;

#if USE_PSRAM
StaticRingbuffer_t *ringbuf_struct;
uint8_t *ringbuf_storage;

static RingbufHandle_t spiramRingbuff(size_t buff_len){
	ringbuf_struct=(StaticRingbuffer_t*)heap_caps_malloc(sizeof(StaticRingbuffer_t),MALLOC_CAP_SPIRAM);
	ringbuf_storage=(uint8_t*)heap_caps_malloc(buff_len,MALLOC_CAP_SPIRAM);
	return xRingbufferCreateStatic(buff_len,RINGBUF_TYPE_BYTEBUF,ringbuf_storage,ringbuf_struct);

}
static void free_spiramRingbuff(RingbufHandle_t handle){
	vRingbufferDelete(handle);
	heap_caps_free(ringbuf_struct);
	heap_caps_free(ringbuf_storage);
}

#define mp3_malloc(val)	heap_caps_malloc(val,MALLOC_CAP_SPIRAM)
#define mp3_free(ptr)	heap_caps_free(ptr)
#else
#define mp3_malloc(val)	malloc(val)
#define mp3_free(ptr)	free(ptr)
#endif

#define EVT_DECODE_STOP	BIT0
#define EVT_PLAY_STOP	BIT1
static EventGroupHandle_t mp3_evt;

mp3_status_t mp3_getStatus(){
	return MP3_STATUS;
}
void i2s_write_task(void* arg){
	size_t len=0;
	while(1){
		void* ptr=xRingbufferReceiveUpTo(ringbuf,&len,portMAX_DELAY,pcmsize);
		memcpy(playbuffer,ptr,pcmsize);
		vRingbufferReturnItem(ringbuf, ptr);
		i2s_write(0,playbuffer,len,&len,portMAX_DELAY);
		if((xEventGroupGetBits(mp3_evt) & EVT_PLAY_STOP) == EVT_PLAY_STOP){
			break;
		}
	}
	size_t max = xRingbufferGetMaxItemSize(ringbuf);

	while(1){
		size_t freeS = xRingbufferGetCurFreeSize(ringbuf);
		void* ptr;

		if(max<=freeS) break;
		else if(max-freeS<pcmsize){
			ptr=xRingbufferReceiveUpTo(ringbuf,&len,5/portTICK_PERIOD_MS,max-freeS);
		}else{
			ptr=xRingbufferReceiveUpTo(ringbuf,&len,5/portMAX_DELAY,pcmsize);
		}
		memcpy(playbuffer,ptr,len);
		vRingbufferReturnItem(ringbuf, ptr);

		i2s_write(0,playbuffer,len,&len,portMAX_DELAY);
	}
	memset(playbuffer,0,pcmsize);
	i2s_write(0,playbuffer,pcmsize,&len,portMAX_DELAY);
	i2s_write(0,playbuffer,pcmsize,&len,portMAX_DELAY);

#if USE_PSRAM
	free_spiramRingbuff(ringbuf);
#else
	vRingbufferDelete(ringbuf);
#endif

	MP3FreeDecoder(hMP3Decoder);
	mp3_free(mp3buff);
	mp3_free(pcmbuffer);
	mp3_free(playbuffer);

	if(mp3_inbuffer!=NULL){ESP_LOGI(TAG,"freeMP3Buff");mp3_free(mp3_inbuffer);mp3_inbuffer=NULL;}

	codec_deinit();
	vEventGroupDelete(mp3_evt);

	ESP_LOGI(TAG,"mp3 i2s Task end.");
	MP3_STATUS=MP3_STOP;
	vTaskDelete(NULL);
}

void mp3decode_task(void* arg){
	mp3_evt=xEventGroupCreate();
	MP3_STATUS=MP3_PLAY;
	//버퍼가 가득찰때까지 데이터를 받음.
	mp3buff=(uint8_t*)mp3_malloc(IN_BUFF_SIZE);
	uint16_t bytesLeft=0;

	uint32_t aaa=0;
	while(bytesLeft<IN_BUFF_SIZE){
		bytesLeft+=callback(mp3buff+bytesLeft,IN_BUFF_SIZE-bytesLeft);
		ESP_LOGI(TAG,"...");
	}
	ESP_LOGI(TAG,"memfill=%d",bytesLeft);
	aaa+=bytesLeft;

	uint8_t* readptr=mp3buff;
	uint8_t* buffend=mp3buff+IN_BUFF_SIZE;
	uint8_t* writeptr=mp3buff+bytesLeft;
	bool isPlaying=false;

    hMP3Decoder = MP3InitDecoder();	//mp3 초기화
    MP3FrameInfo frameInfo;

    int errcnt=0;
	while(1){
		if(xEventGroupGetBits(mp3_evt)==EVT_DECODE_STOP)break;
		//버퍼에 빈공간 만큼 쉬프트 시킨다.
		if((readptr!=mp3buff)&&(bytesLeft>0)){
			memmove(mp3buff,readptr,bytesLeft);
			readptr= mp3buff;
			writeptr = mp3buff+bytesLeft;
		}
		//데이터를 받아 온다.
		if((buffend-writeptr)>0){
			int bytesRead = callback(writeptr,buffend-writeptr);
			if(bytesRead==MP3_CALLBACK_END){
				ESP_LOGI(TAG,"bytesLeft=%d",bytesLeft);
				if(bytesLeft==0){
					//vTaskDelay(100/portTICK_PERIOD_MS);
					break;
				}
			}
			else if(bytesRead==MP3_CALLBACK_REPEAT){
				ESP_LOGI(TAG,"bytesLeft=%d",bytesLeft);
				if(bytesLeft==0){
					//vTaskDelay(100/portTICK_PERIOD_MS);
					break;
				}
			}
			else if(bytesRead==MP3_CALLBACK_DATAEND){
				ESP_LOGI(TAG,"bytesLeft=%d",bytesLeft);
				if(bytesLeft==0){
					//vTaskDelay(100/portTICK_PERIOD_MS);
					break;
				}
			}else{
				writeptr += bytesRead;
				bytesLeft += bytesRead;
				aaa+=bytesRead;
				//ESP_LOGI(TAG,"read=%d,tt=%d",bytesRead,aaa);
			}
		}
		//아직 프레임헤터를 읽지 못한 경우 헤더를 읽는다.
		int err, offset;
		if(!isPlaying){
			offset = MP3FindSyncWord(readptr,bytesLeft);
			if(offset>=0){
				readptr += offset;
				bytesLeft -= offset;
			}
			if (bytesLeft < sizeof(MP3FrameInfo))continue;
			err = MP3GetNextFrameInfo(hMP3Decoder,&frameInfo,readptr);
			if (err == ERR_MP3_INVALID_FRAMEHEADER) {
				readptr += 1;
				bytesLeft -= 1;
			}else{
				ESP_LOGI(TAG,"bitrate=%d",frameInfo.bitrate);
				ESP_LOGI(TAG,"bitsPerSample=%d",frameInfo.bitsPerSample);
				ESP_LOGI(TAG,"layer=%d",frameInfo.layer);
				ESP_LOGI(TAG,"nChans=%d",frameInfo.nChans);
				ESP_LOGI(TAG,"outputSamps=%d",frameInfo.outputSamps);
				ESP_LOGI(TAG,"samprate=%d",frameInfo.samprate);
				ESP_LOGI(TAG,"version=%d",frameInfo.version);
				isPlaying=true;
				codec_init(frameInfo.samprate,frameInfo.bitsPerSample,frameInfo.nChans);
				pcmsize=frameInfo.outputSamps*frameInfo.bitsPerSample/8;
				pcmbuffer=mp3_malloc(pcmsize);
				playbuffer=mp3_malloc(pcmsize);
#if USE_PSRAM
				ringbuf=spiramRingbuff(pcmsize*2);
#else
				ringbuf=xRingbufferCreate(pcmsize*2, RINGBUF_TYPE_BYTEBUF);
#endif
				xTaskCreate(i2s_write_task, "ii2s", 4096,NULL, configMAX_PRIORITIES - 3, &i2s_handle);
			}
			continue;
		}else{ //재생코드
			offset = MP3FindSyncWord(readptr, bytesLeft);
			if(offset>=0){
				if(offset>0)ESP_LOGE(TAG,"offset=%d!!",offset);
				readptr+=offset;
				bytesLeft-=offset;
				err=MP3Decode(hMP3Decoder,&readptr,(int*)&bytesLeft,pcmbuffer,0);
				if(err){
					if(err == ERR_MP3_INDATA_UNDERFLOW ){
						break;
					}
					else if (err == ERR_MP3_INVALID_FRAMEHEADER) {
						readptr += 1;
						bytesLeft -= 1;
						ESP_LOGE(TAG,"invalid header");
						continue;
					}
					else ESP_LOGE(TAG,"err=%d",err);
				}else{
					MP3DecInfo *mp3DecInfo = (MP3DecInfo *)hMP3Decoder;
					xRingbufferSend(ringbuf,pcmbuffer,mp3DecInfo->nGrans * mp3DecInfo->nGranSamps * mp3DecInfo->nChans * frameInfo.bitsPerSample/8,portMAX_DELAY);
				}
			}else{
				if(++errcnt>2)break;
			}
		}

	}
	//xEventGroupSetBits(mp3_evt,EVT_PLAY_STOP);
	ESP_LOGI(TAG,"total Data=%d",aaa);
	xEventGroupSetBits(mp3_evt,EVT_PLAY_STOP);
//	vTaskDelay(100/portTICK_PERIOD_MS);

	ESP_LOGI(TAG,"mp3 decode Task end.");
	vTaskDelete(NULL);
}


void mp3_start(int32_t (*datacallback)(uint8_t *buf, int len)){
	callback=datacallback;
	xTaskCreate(mp3decode_task, "sdmp3", 4096,NULL, tskIDLE_PRIORITY+2, &decode_handle) ;
}

void mp3_pause(){
	codec_off();
	vTaskSuspend(decode_handle);
	MP3_STATUS=MP3_PAUSE;
}

void mp3_resume(){
	codec_on();
	vTaskResume(decode_handle);
	MP3_STATUS=MP3_PLAY;
}

static FILE* fd=NULL;

void mp3_stop(){

	xEventGroupSetBits(mp3_evt,EVT_DECODE_STOP);
	while(mp3_getStatus()==MP3_PLAY){vTaskDelay(50/portTICK_PERIOD_MS);}

	if(mp3_inbuffer!=NULL){ESP_LOGI(TAG,"freeMP3Buff");mp3_free(mp3_inbuffer);mp3_inbuffer=NULL;}
	if(fd!=NULL){fclose(fd);fd=NULL;}
	MP3_STATUS=MP3_STOP;
}

static int32_t mp3Filecallback(uint8_t *buf, int len){
	if(fd==NULL) return MP3_CALLBACK_DATAEND;
	len=fread(buf,1,len,fd);
	if(len<=0){
		fclose(fd);
		fd=NULL;
		return MP3_CALLBACK_DATAEND;
	}
	return len;
}

void mp3_play(const char* path){
	fd = fopen(path,"rb");
	if(fd==NULL){
		ESP_LOGE(TAG,"fopen fail.");
		return;
	}
	mp3_start(mp3Filecallback);
}


static size_t mempoz=0;
static size_t filelength=0;
static bool bloop=false;
static int32_t mp3_memcpy_callback(uint8_t *buf, int len){
	if(mempoz>=filelength){
		mempoz=0;
		if(bloop)return MP3_CALLBACK_REPEAT;
		else return MP3_CALLBACK_END;
	}
	if(mempoz+len>filelength)len=filelength-mempoz;
	memcpy(buf,mp3_inbuffer+mempoz,len);
	mempoz+=len;
	return len;
}

void mp3_memcpy_loopplay(const char* path){

	bloop=true;
	mempoz=0;
    FILE *fp = fopen(path,"rb");
    ESP_LOGI(TAG,"%s Open",path);
    if(fp==NULL){
    	ESP_LOGI(TAG,"File Open Fail");
    	return ;
    }
    fseek(fp,0,SEEK_END);
    filelength = ftell(fp);
    fseek(fp,0,SEEK_SET);
    //ESP_LOGI(TAG,"size=%d",filelength);
    mp3_inbuffer=mp3_malloc(filelength);

    uint16_t len=0;
    while(mempoz<filelength){
    	len=fread((void*)(mp3_inbuffer+mempoz),1,4096,fp);
    	//ESP_LOGI(TAG,"len=%d,size=%d",len,mempoz);
    	mempoz+=len;
    }
    fclose(fp);
    mempoz=0;

	mp3_start(mp3_memcpy_callback);
}


static const uint8_t* music_mp3_start;
static const uint8_t* music_mp3_end;
static const uint8_t* ptrPos=0;
//as
static int32_t mp3_play_flash_callback(uint8_t *buf, int len){
	if(music_mp3_end<=ptrPos){
		return MP3_CALLBACK_DATAEND;
		ESP_LOGI(TAG,"sdas");
	}
	else if(music_mp3_end-ptrPos>len){
		memcpy(buf,ptrPos,len);
		ptrPos+=len;
		ESP_LOGI(TAG,"len=%d",len);
		return len;
	}else{
		uint32_t llen=(uint32_t)(music_mp3_end-ptrPos);
		memcpy(buf,ptrPos,llen);
		ptrPos+=llen;
		ESP_LOGI(TAG,"len=%d",llen);
		if(llen==0)return MP3_CALLBACK_DATAEND;
		return llen;
	}
}

void mp3_play_flash(const uint8_t* st,const uint8_t* ed){
	music_mp3_start=ptrPos=st;
	music_mp3_end=ed;
	mp3_start(mp3_play_flash_callback);
}
