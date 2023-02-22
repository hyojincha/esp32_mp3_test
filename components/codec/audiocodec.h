/*
 * audiocodec.h
 *
 *  Created on: 2020. 6. 2.
 *      Author: Owner
 */

#ifndef COMPONENTS_CODEC_AUDIOCODEC_H_
#define COMPONENTS_CODEC_AUDIOCODEC_H_
#include "driver/i2s.h"
#include "es8388.h"

esp_err_t codec_init(uint32_t samplate,i2s_bits_per_sample_t bitsPerSample,i2s_channel_t nChans);
esp_err_t codec_deinit();
esp_err_t codec_set_clock(uint32_t samplate,i2s_bits_per_sample_t bitsPerSample,i2s_channel_t nChans);
esp_err_t codec_on();
esp_err_t codec_off();
#endif /* COMPONENTS_CODEC_AUDIOCODEC_H_ */
