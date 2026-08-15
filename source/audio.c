/*
 * audio.c — Multi-voice Ogg Vorbis Audio Mixer for Advena (PS Vita)
 */

#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <tremor/ivorbisfile.h>

#include "audio.h"
#include "utils/logger.h"

#define SND_DIR1 "ux0:data/advena/sound"
#define SND_DIR2 "ux0:data/advena/res/raw"
#define SND_DIR3 "ux0:data/advena/assets/sound"

#define AUDIO_RATE 44100
#define AUDIO_GRAIN 512

#define VOICE_BGM    0
#define VOICE_STREAM 1
#define VOICE_SFX0   2
#define NUM_VOICES   8

typedef struct {
    OggVorbis_File vf;
    int active;
    int loop;
    int channels;
    long rate;
    float gain;
    float pos_frac;
    int16_t prev_l, prev_r;
    int have_prev;
    int16_t stage[256];
    int stage_frames, stage_pos;
} voice_t;

static voice_t voices[NUM_VOICES];
static SceUID audio_mutex = -1;
static SceUID audio_thread_id = -1;
static int audio_port = -1;
static volatile int audio_running = 0;

static void voice_close(voice_t *v) {
    if (v->active) {
        ov_clear(&v->vf);
        v->active = 0;
    }
}

static int voice_next_src_frame(voice_t *v, int16_t *l, int16_t *r) {
    while (v->stage_pos >= v->stage_frames) {
        int bs;
        long got = ov_read(&v->vf, (char *) v->stage, sizeof(v->stage), &bs);
        if (got <= 0) {
            if (v->loop && got == 0 && ov_pcm_seek(&v->vf, 0) == 0)
                continue;
            voice_close(v);
            return 0;
        }
        v->stage_frames = (int) got / (v->channels * 2);
        v->stage_pos = 0;
    }

    *l = v->stage[v->stage_pos * v->channels];
    *r = v->stage[v->stage_pos * v->channels + (v->channels > 1 ? 1 : 0)];
    v->stage_pos++;
    return 1;
}

static int voice_decode(voice_t *v, int16_t *out, int frames) {
    int done = 0;

    if (v->rate == AUDIO_RATE) {
        int16_t l, r;
        while (done < frames && v->active) {
            if (!voice_next_src_frame(v, &l, &r)) break;
            out[done * 2]     = (int16_t)(l * v->gain);
            out[done * 2 + 1] = (int16_t)(r * v->gain);
            done++;
        }
        return done;
    }

    float step = (float) v->rate / (float) AUDIO_RATE;
    while (done < frames && v->active) {
        while (v->pos_frac >= 1.0f && v->active) {
            v->prev_l = v->stage[v->stage_pos * v->channels];
            v->prev_r = v->stage[v->stage_pos * v->channels + (v->channels > 1 ? 1 : 0)];
            v->have_prev = 1;
            int16_t dummy_l, dummy_r;
            if (!voice_next_src_frame(v, &dummy_l, &dummy_r)) break;
            v->pos_frac -= 1.0f;
        }
        if (!v->active) break;

        int16_t cur_l = v->stage[v->stage_pos * v->channels];
        int16_t cur_r = v->stage[v->stage_pos * v->channels + (v->channels > 1 ? 1 : 0)];
        float p0_l = v->have_prev ? (float) v->prev_l : (float) cur_l;
        float p0_r = v->have_prev ? (float) v->prev_r : (float) cur_r;
        float frac = v->pos_frac;

        float samp_l = (p0_l + frac * ((float) cur_l - p0_l)) * v->gain;
        float samp_r = (p0_r + frac * ((float) cur_r - p0_r)) * v->gain;

        if (samp_l > 32767.0f) samp_l = 32767.0f;
        else if (samp_l < -32768.0f) samp_l = -32768.0f;
        if (samp_r > 32767.0f) samp_r = 32767.0f;
        else if (samp_r < -32768.0f) samp_r = -32768.0f;

        out[done * 2]     = (int16_t) samp_l;
        out[done * 2 + 1] = (int16_t) samp_r;
        done++;
        v->pos_frac += step;
    }
    return done;
}

static int audio_thread(SceSize args, void *argp) {
    int32_t mix_buf[AUDIO_GRAIN * 2];
    int16_t out_buf[AUDIO_GRAIN * 2];
    int16_t voice_buf[AUDIO_GRAIN * 2];

    while (audio_running) {
        memset(mix_buf, 0, sizeof(mix_buf));

        sceKernelLockMutex(audio_mutex, 1, NULL);
        for (int i = 0; i < NUM_VOICES; i++) {
            if (!voices[i].active) continue;
            int got = voice_decode(&voices[i], voice_buf, AUDIO_GRAIN);
            for (int k = 0; k < got * 2; k++) {
                mix_buf[k] += voice_buf[k];
            }
        }
        sceKernelUnlockMutex(audio_mutex, 1);

        for (int k = 0; k < AUDIO_GRAIN * 2; k++) {
            int32_t v = mix_buf[k];
            if (v > 32767) v = 32767;
            else if (v < -32768) v = -32768;
            out_buf[k] = (int16_t) v;
        }

        sceAudioOutOutput(audio_port, out_buf);
    }
    return 0;
}

void audio_init(void) {
    if (audio_running) return;

    memset(voices, 0, sizeof(voices));
    audio_mutex = sceKernelCreateMutex("advena_audio_mtx", 0, 0, NULL);
    audio_port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, AUDIO_GRAIN, AUDIO_RATE, SCE_AUDIO_OUT_MODE_STEREO);
    if (audio_port < 0) {
        l_error("[Audio] Failed to open audio port: 0x%08x", audio_port);
        return;
    }

    int vol[2] = { SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB };
    sceAudioOutSetVolume(audio_port, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vol);

    audio_running = 1;
    audio_thread_id = sceKernelCreateThread("advena_audio_thread", audio_thread, 0x10000100, 0x10000, 0, 0, NULL);
    if (audio_thread_id >= 0) {
        sceKernelStartThread(audio_thread_id, 0, NULL);
        l_success("[Audio] Audio mixer initialized successfully.");
    }
}

static int resolve_sound_path(int snd_id, char *out, size_t out_size) {
    snprintf(out, out_size, "%s/s%03d.ogg", SND_DIR1, snd_id);
    if (access(out, F_OK) == 0) return 1;

    snprintf(out, out_size, "%s/s%03d.ogg", SND_DIR2, snd_id);
    if (access(out, F_OK) == 0) return 1;

    snprintf(out, out_size, "%s/s%03d.ogg", SND_DIR3, snd_id);
    if (access(out, F_OK) == 0) return 1;

    snprintf(out, out_size, "%s/%d.ogg", SND_DIR1, snd_id);
    if (access(out, F_OK) == 0) return 1;

    return 0;
}

void audio_play(int snd_id, int vol, int is_loop) {
    if (!audio_running || snd_id < 0) return;

    if (vol == 0 && is_loop) {
        audio_stop_bgm();
        return;
    }

    char path[256];
    if (!resolve_sound_path(snd_id, path, sizeof(path))) {
        l_warn("[Audio] Sound file not found for id=%d", snd_id);
        return;
    }

    FILE *f = fopen(path, "rb");
    if (!f) return;

    OggVorbis_File vf;
    if (ov_open(f, &vf, NULL, 0) < 0) {
        fclose(f);
        l_warn("[Audio] Failed to open Ogg stream: %s", path);
        return;
    }

    vorbis_info *vi = ov_info(&vf, -1);
    if (!vi || (vi->channels != 1 && vi->channels != 2)) {
        ov_clear(&vf);
        return;
    }

    int voice_idx;
    if (is_loop) {
        voice_idx = VOICE_BGM;
    } else {
        static int next_sfx = 0;
        voice_idx = VOICE_SFX0 + (next_sfx++ % (NUM_VOICES - VOICE_SFX0));
    }

    sceKernelLockMutex(audio_mutex, 1, NULL);
    voice_close(&voices[voice_idx]);
    voices[voice_idx].vf = vf;
    voices[voice_idx].active = 1;
    voices[voice_idx].loop = is_loop;
    voices[voice_idx].channels = vi->channels;
    voices[voice_idx].rate = vi->rate;
    voices[voice_idx].gain = (vol <= 0) ? 0.8f : (vol / 100.0f);
    voices[voice_idx].pos_frac = 0.0f;
    voices[voice_idx].have_prev = 0;
    voices[voice_idx].stage_frames = 0;
    voices[voice_idx].stage_pos = 0;
    sceKernelUnlockMutex(audio_mutex, 1);
}

void audio_stop_bgm(void) {
    if (!audio_running) return;
    sceKernelLockMutex(audio_mutex, 1, NULL);
    voice_close(&voices[VOICE_BGM]);
    sceKernelUnlockMutex(audio_mutex, 1);
}

void audio_stop_all(void) {
    if (!audio_running) return;
    sceKernelLockMutex(audio_mutex, 1, NULL);
    for (int i = 0; i < NUM_VOICES; i++) {
        voice_close(&voices[i]);
    }
    sceKernelUnlockMutex(audio_mutex, 1);
}
