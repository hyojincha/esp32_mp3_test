/*
 * wavplayer.h
 *
 *  Created on: 2020. 6. 15.
 *      Author: Owner
 */

#ifndef COMPONENTS_MP3PLAYER_WAVPLAYER_H_
#define COMPONENTS_MP3PLAYER_WAVPLAYER_H_

typedef struct{
	char chunkID[4];
	uint32_t chunksize;
	char format[4];

	char chunkID2[4];
	uint32_t chunksize2;
	uint16_t audioformat;
	uint16_t num_of_channel;
	uint32_t samplate;
	uint32_t byterate;
	uint16_t blockAlign;
	uint16_t bitperSample;

	char chunkID3[4];
	uint32_t chunksize3;

}wavheader ;

void wav_start(int32_t (*datacallback)(uint8_t *buf, int len));

#endif /* COMPONENTS_MP3PLAYER_WAVPLAYER_H_ */
