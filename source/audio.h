#ifndef __AUDIO_H__
#define __AUDIO_H__

/**
 * @brief audio.h — Audio subsystem for Advena (PS Vita) Implements BGM and SFX mixing using Tremor (libvorbisidec) and SceAudioOut.
 * @note See `docs/source/audio.md:4` for detailed design rationale.
 */

void audio_init(void);
void audio_play(int snd_id, int vol, int is_loop);
void audio_stop_all(void);
void audio_stop_bgm(void);

#endif
