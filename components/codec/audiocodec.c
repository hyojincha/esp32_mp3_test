#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "es8388.h"
#include "audiocodec.h"
#include "pindefine.h"
esp_err_t codec_init(uint32_t samplate,i2s_bits_per_sample_t bitsPerSample,i2s_channel_t nChans){
	i2s_config_t i2s_conf={
			.mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX ,
		    .sample_rate = 44100,
			.bits_per_sample = 16,
			.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
			.communication_format = I2S_COMM_FORMAT_I2S,
			.dma_buf_count = 6,
			.dma_buf_len = 60,
			.use_apll=1,
			.intr_alloc_flags=ESP_INTR_FLAG_LEVEL2,
			.fixed_mclk=0,
			.tx_desc_auto_clear = true
	};
	if(nChans<2)i2s_conf.channel_format=I2S_CHANNEL_FMT_ONLY_RIGHT;
    i2s_pin_config_t i2s_pin_cfg={
    		.bck_io_num=I2S_PIN_BCK,
			.ws_io_num=I2S_PIN_WS,
			.data_out_num=I2S_PIN_DOUT,
			.data_in_num=I2S_PIN_DIN
    };
    i2s_driver_install(I2S_NUM_0, &i2s_conf, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &i2s_pin_cfg);
	i2s_zero_dma_buffer(I2S_NUM_0);
	i2s_set_clk(I2S_NUM_0,samplate,bitsPerSample,nChans);

	SET_PERI_REG_BITS(PIN_CTRL, CLK_OUT1, 0, CLK_OUT1_S);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);

	audio_hal_codec_config_t acfg;
	acfg.adc_input  = AUDIO_HAL_ADC_INPUT_ALL;
	acfg.dac_output = AUDIO_HAL_DAC_OUTPUT_ALL;
	acfg.codec_mode = AUDIO_HAL_CODEC_MODE_BOTH;
	acfg.i2s_iface.mode = AUDIO_HAL_MODE_SLAVE;
	acfg.i2s_iface.fmt = AUDIO_HAL_I2S_NORMAL;;
	acfg.i2s_iface.samples = AUDIO_HAL_48K_SAMPLES;
	acfg.i2s_iface.bits = bitsPerSample/8;

	es8388_init(&acfg);
	es8388_start(ES_MODULE_ADC_DAC);
	//es8388_set_voice_volume(50);

    es8388_pa_power(true);
	return ESP_OK;
}

esp_err_t codec_off(){
	es8388_pa_power(false);
	return ESP_OK;
}

esp_err_t codec_on(){
	es8388_pa_power(true);
	return ESP_OK;
}

esp_err_t codec_set_clock(uint32_t samplate,i2s_bits_per_sample_t bitsPerSample,i2s_channel_t nChans){
	i2s_set_clk(I2S_NUM_0,samplate,bitsPerSample,nChans);
	return ESP_OK;
}

esp_err_t codec_deinit(){
	es8388_pa_power(false);
	i2s_driver_uninstall(I2S_NUM_0);
	es8388_stop(ES_MODULE_ADC_DAC);
	es8388_deinit();
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_GPIO0);
	return ESP_OK;
}
