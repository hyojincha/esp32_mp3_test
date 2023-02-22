/*
 * mp3player.h
 *
 *  Created on: 2020. 6. 5.
 *      Author: Owner
 */

#ifndef COMPONENTS_MP3PLAYER_MP3PLAYER_H_
#define COMPONENTS_MP3PLAYER_MP3PLAYER_H_

typedef enum {
	MP3_STOP,
	MP3_PAUSE,
	MP3_PLAY
}mp3_status_t;

void mp3_start(int32_t (*datacallback)(uint8_t *buf, int len));

void mp3_play(const char* path);
void mp3_memcpy_loopplay(const char* path);
void mp3_play_flash(const uint8_t* st,const uint8_t* ed);

void mp3_pause();
void mp3_resume();
void mp3_stop();
mp3_status_t mp3_getStatus();

#endif /* COMPONENTS_MP3PLAYER_MP3PLAYER_H_ */
