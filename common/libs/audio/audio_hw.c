/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "audio_hw_primary"
/*#define LOG_NDEBUG 0*/

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>

#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

#include <expat.h>

#include <tinyalsa/asoundlib.h>
#include <audio_utils/resampler.h>
#include <audio_utils/echo_reference.h>
#include <hardware/audio_effect.h>
#include <audio_effects/effect_aec.h>
#include "audio_pga.h"
#include "vb_effect_if.h"
#include "vb_pga.h"

#include "eng_audio.h"
#include "aud_proc.h"

//#define XRUN_DEBUG

#ifdef XRUN_DEBUG
#define XRUN_TRACE  ALOGW
#else
#define XRUN_TRACE
#endif
#define BLUE_TRACE  ALOGW

//#define AUDIO_DUMP
#define AUDIO_OUT_FILE_PATH    "data/audio_out.pcm"

#define PRIVATE_NAME_LEN 60

#define CTL_TRACE(exp) ALOGW(#exp" is %s", ((exp) != NULL) ? "successful" : "failure")

#define PRIVATE_MIC_BIAS                  "mic bias"
#define PRIVATE_VBC_CONTROL              "vb control"
#define PRIVATE_VBC_EQ_SWITCH            "eq switch"
#define PRIVATE_VBC_EQ_UPDATE            "eq update"
#define PRIVATE_VBC_EQ_PROFILE            "eq profile"
#define PRIVATE_INTERNAL_PA              "internal PA"

/* ALSA cards for sprd */
#define CARD_SPRDPHONE "sprdphone"
#define CARD_VAUDIO    "VIRTUAL AUDIO"

/* ALSA ports for sprd */
#define PORT_MM 0
#define PORT_MODEM 1

/* constraint imposed by VBC: all period sizes must be multiples of 160 */
#define VBC_BASE_FRAME_COUNT 160
/* number of base blocks in a short period (low latency) */
#define SHORT_PERIOD_MULTIPLIER 10  /* 36 ms */
/* number of frames per short period (low latency) */
#define SHORT_PERIOD_SIZE (VBC_BASE_FRAME_COUNT * SHORT_PERIOD_MULTIPLIER)
/* number of short periods in a long period (low power) */
#define LONG_PERIOD_MULTIPLIER 8  /* 304 ms */
/* number of frames per long period (low power) */
#define LONG_PERIOD_SIZE (SHORT_PERIOD_SIZE * LONG_PERIOD_MULTIPLIER)
/* number of periods for low power playback */
#define PLAYBACK_LONG_PERIOD_COUNT 2
/* number of pseudo periods for low latency playback */
#define PLAYBACK_SHORT_PERIOD_COUNT 4
/* number of periods for capture */
#define CAPTURE_PERIOD_COUNT 2
/* minimum sleep time in out_write() when write threshold is not reached */
#define MIN_WRITE_SLEEP_US 5000

#define RESAMPLER_BUFFER_FRAMES (SHORT_PERIOD_SIZE * 2)
#define RESAMPLER_BUFFER_SIZE (4 * RESAMPLER_BUFFER_FRAMES)

#define DEFAULT_OUT_SAMPLING_RATE 44100
#define DEFAULT_IN_SAMPLING_RATE  8000

/* sampling rate when using MM low power port */
#define MM_LOW_POWER_SAMPLING_RATE 44100
/* sampling rate when using MM full power port */
#define MM_FULL_POWER_SAMPLING_RATE 48000
/* sampling rate when using VX port for narrow band */
#define VX_NB_SAMPLING_RATE 8000
/* sampling rate when using VX port for wide band */
#define VX_WB_SAMPLING_RATE 16000

struct pcm_config pcm_config_mm = {
    .channels = 2,
    .rate = DEFAULT_OUT_SAMPLING_RATE,
    .period_size = LONG_PERIOD_SIZE,
    .period_count = PLAYBACK_LONG_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_mm_ul = {
    .channels = 2,
    .rate = MM_FULL_POWER_SAMPLING_RATE,
    .period_size = SHORT_PERIOD_SIZE,
    .period_count = CAPTURE_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_vx = {
    .channels = 2,
    .rate = VX_NB_SAMPLING_RATE,
    .period_size = VBC_BASE_FRAME_COUNT,
    .period_count = 2,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_vrec_vx = {    //voice record in vlx mode
    .channels = 1,
    .rate = VX_NB_SAMPLING_RATE,
    .period_size = 320,
    .period_count = 8,
    .format = PCM_FORMAT_S16_LE,
};


struct pcm_config pcm_config_vplayback = {
    .channels = 1,
    .rate = VX_NB_SAMPLING_RATE,
    .period_size = 320,
    .period_count = 8,
    .format = PCM_FORMAT_S16_LE,
};



#define MIN(x, y) ((x) > (y) ? (y) : (x))

struct route_setting
{
    char *ctl_name;
    int intval;
    char *strval;
};

struct tiny_dev_cfg {
    int mask;

    struct route_setting *on;
    unsigned int on_len;

    struct route_setting *off;
    unsigned int off_len;
};

struct tiny_private_ctl {
    struct mixer_ctl *mic_bias_switch;
    struct mixer_ctl *vbc_switch;
    struct mixer_ctl *vbc_eq_switch;
    struct mixer_ctl *vbc_eq_update;
    struct mixer_ctl *vbc_eq_profile_select;
    struct mixer_ctl *internal_pa;
};

struct stream_routing_manager {
    pthread_t        routing_switch_thread;
    bool             is_exit;
    sem_t           device_switch_sem;
};

struct tiny_audio_device {
    struct audio_hw_device hw_device;

    pthread_mutex_t lock;       /* see note below on mutex acquisition order */
    struct mixer *mixer;
    audio_mode_t mode;
    int devices;
    struct pcm *pcm_modem_dl;
    struct pcm *pcm_modem_ul;
    int call_start;
    int call_connected;
    int call_prestop;
    float voice_volume;
    struct tiny_stream_in *active_input;
    struct tiny_stream_out *active_output;
    bool mic_mute;
    struct echo_reference_itfe *echo_reference;
    bool bluetooth_nrec;
    bool low_power;

    struct tiny_dev_cfg *dev_cfgs;
    unsigned int num_dev_cfgs;

    struct tiny_private_ctl private_ctl;
    struct audio_pga *pga;
    bool eq_available;

    struct stream_routing_manager  routing_mgr;
};

struct tiny_stream_out {
    struct audio_stream_out stream;

    pthread_mutex_t lock;       /* see note below on mutex acquisition order */
    struct pcm_config config;
    struct pcm *pcm;
    struct pcm *pcm_vplayback;
    struct resampler_itfe  *resampler_vplayback;
    struct resampler_itfe *resampler;
    char *buffer;
    char * buffer_vplayback;
    int standby;
    struct echo_reference_itfe *echo_reference;
    struct tiny_audio_device *dev;
    int write_threshold;
    bool low_power;
    FILE * out_dump_fd;
};

#define MAX_PREPROCESSORS 3 /* maximum one AGC + one NS + one AEC per input stream */

struct tiny_stream_in {
    struct audio_stream_in stream;

    pthread_mutex_t lock;       /* see note below on mutex acquisition order */
    struct pcm_config config;
    struct pcm *pcm;
    int device;
    struct resampler_itfe *resampler;
    struct resampler_buffer_provider buf_provider;
    int16_t *buffer;
    size_t frames_in;
    unsigned int requested_rate;
    int standby;
    int source;
    struct echo_reference_itfe *echo_reference;
    bool need_echo_reference;
    effect_handle_t preprocessors[MAX_PREPROCESSORS];
    int num_preprocessors;
    int16_t *proc_buf;
    size_t proc_buf_size;
    size_t proc_frames_in;
    int16_t *ref_buf;
    size_t ref_buf_size;
    size_t ref_frames_in;
    int read_status;

    struct tiny_audio_device *dev;
    int active_rec_proc;
};

struct config_parse_state {
    struct tiny_audio_device *adev;
    struct tiny_dev_cfg *dev;
    bool on;

    struct route_setting *path;
    unsigned int path_len;

    char private_name[PRIVATE_NAME_LEN];
};

static const struct {
    int mask;
    const char *name;
} dev_names[] = {
    { AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_FM_SPEAKER, "speaker" },
    { AUDIO_DEVICE_OUT_WIRED_HEADSET | AUDIO_DEVICE_OUT_WIRED_HEADPHONE |AUDIO_DEVICE_OUT_FM_HEADSET,
          "headphone" },
    { AUDIO_DEVICE_OUT_EARPIECE, "earpiece" },
    /* ANLG for voice call via linein*/
    { AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET | AUDIO_DEVICE_OUT_ALL_FM, "line" },
    { AUDIO_DEVICE_OUT_FM_HEADSET, "line-headphone" },
    { AUDIO_DEVICE_OUT_FM_SPEAKER, "line-speaker" },

    { AUDIO_DEVICE_IN_COMMUNICATION, "comms" },
    { AUDIO_DEVICE_IN_AMBIENT, "ambient" },
    { AUDIO_DEVICE_IN_BUILTIN_MIC, "builtin-mic" },
    { AUDIO_DEVICE_IN_WIRED_HEADSET, "headset-in" },
    { AUDIO_DEVICE_IN_AUX_DIGITAL, "digital" },
    { AUDIO_DEVICE_IN_BACK_MIC, "back-mic" },
    //{ "linein-capture"},
};
/*
 * card define
 */
static int s_tinycard = 0;
static int s_vaudio = 0;

/**
 * NOTE: when multiple mutexes have to be acquired, always respect the following order:
 *        hw device > in stream > out stream
 */
extern int get_snd_card_number(const char *card_name);
int set_call_route(struct tiny_audio_device *adev, int device, int on);
static void select_devices_signal(struct tiny_audio_device *adev);
static void do_select_devices(struct tiny_audio_device *adev);
static int set_route_by_array(struct mixer *mixer, struct route_setting *route,unsigned int len);
static int adev_set_voice_volume(struct audio_hw_device *dev, float volume);
static int do_input_standby(struct tiny_stream_in *in);
static int do_output_standby(struct tiny_stream_out *out);
static void force_all_standby(struct tiny_audio_device *adev);
static struct route_setting * get_route_setting (
    struct tiny_audio_device *adev,
    int devices,
    int on);

static int get_route_depth (
    struct tiny_audio_device *adev,
    int devices,
    int on);

static int get_mode_from_devices(int devices);
static int init_rec_process(int rec_mode, int sample_rate);
static int aud_rec_do_process(void * buffer, size_t bytes);

static void *stream_routing_thread_entry(void * adev);
static int stream_routing_manager_create(struct tiny_audio_device *adev);
static void stream_routing_manager_close(struct tiny_audio_device *adev);

/*
 * NOTE: audio stream(playback, capture) dump just for debug.
*/
static int out_dump_create(FILE **out_fd, const char *path);
static int out_dump_doing(FILE *out_fd, const void* buffer, size_t bytes);
static int out_dump_release(FILE **fd);

#ifndef _VOICE_CALL_VIA_LINEIN
#include "vb_control_parameters.c"
#endif

#include "at_commands_generic.c"
#include "mmi_audio_loop.c"


static int out_dump_create(FILE **out_fd, const char *path)
{
    if (path == NULL) {
        ALOGE("path not assigned.");
        return -1;
    }
    *out_fd = (FILE *)fopen(path, "wb");
    if (*out_fd == NULL ) {
        ALOGE("cannot create file.");
        return -1;
    }
    ALOGI("path %s created successfully.", path);
    return 0;
}

static int out_dump_doing(FILE *out_fd, const void* buffer, size_t bytes)
{
    int ret;
    if (out_fd) {
        ret = fwrite((uint8_t *)buffer, bytes, 1, out_fd);
        if (ret < 0) ALOGW("%d, fwrite failed.", bytes);
    } else {
       ALOGW("out_fd is NULL, cannot write.");
    }
    return 0;
}

static int out_dump_release(FILE **fd)
{
    fclose(*fd);
    *fd = NULL;
    return 0;
}


int set_call_route(struct tiny_audio_device *adev, int device, int on)
{
    struct route_setting *cur_setting;
    int cur_depth = 0;

    cur_setting = get_route_setting(adev, device, on);
    cur_depth = get_route_depth(adev, device, on);
    if (adev->mixer && cur_setting)
        set_route_by_array(adev->mixer, cur_setting, cur_depth);
#ifdef _VOICE_CALL_VIA_LINEIN
    //open Mic Bias
    mixer_ctl_set_value(adev->private_ctl.mic_bias_switch, 0, on);
#endif
    return 0;
}

static struct route_setting * get_route_setting(
    struct tiny_audio_device *adev,
    int devices,
    int on)
{
    unsigned int i = 0;
    for (i=0; i<adev->num_dev_cfgs; i++) {
        if (devices & adev->dev_cfgs[i].mask) {
            if (on)
                return adev->dev_cfgs[i].on;
            else
                return adev->dev_cfgs[i].off;
        }
    }
    ALOGW("[get_route_setting], warning: devices(0x%08x) NOT found.", devices);
    return NULL;
}

static int get_route_depth (
    struct tiny_audio_device *adev,
    int devices,
    int on)
{
    unsigned int i = 0;
    for (i=0; i<adev->num_dev_cfgs; i++) {
        if (devices & adev->dev_cfgs[i].mask) {
            if (on)
                return adev->dev_cfgs[i].on_len;
            else
                return adev->dev_cfgs[i].off_len;
        }
    }
    ALOGW("[get_route_setting], warning: devices(0x%08x) NOT found.", devices);
    return 0;
}

/* The enable flag when 0 makes the assumption that enums are disabled by
 * "Off" and integers/booleans by 0 */
static int set_route_by_array(struct mixer *mixer, struct route_setting *route,
			      unsigned int len)
{
    struct mixer_ctl *ctl;
    unsigned int i, j, ret;

    /* Go through the route array and set each value */
    for (i = 0; i < len; i++) {
        ctl = mixer_get_ctl_by_name(mixer, route[i].ctl_name);
        if (!ctl) {
            ALOGE("Unknown control '%s'\n", route[i].ctl_name);
            continue;
        }

        if (route[i].strval) {
            ret = mixer_ctl_set_enum_by_string(ctl, route[i].strval);
            if (ret != 0) {
                ALOGE("Failed to set '%s' to '%s'\n",
                route[i].ctl_name, route[i].strval);
            } else {
                ALOGI("Set '%s' to '%s'\n",
                route[i].ctl_name, route[i].strval);
            }
        } else {
            /* This ensures multiple (i.e. stereo) values are set jointly */
            for (j = 0; j < mixer_ctl_get_num_values(ctl); j++) {
                ret = mixer_ctl_set_value(ctl, j, route[i].intval);
                if (ret != 0) {
                    ALOGE("Failed to set '%s'.%d to %d\n",
                    route[i].ctl_name, j, route[i].intval);
                } else {
                    ALOGI("Set '%s'.%d to %d\n",
                    route[i].ctl_name, j, route[i].intval);
                }
	        }
        }
    }

    return 0;
}

/* Must be called with route_lock */
static void do_select_devices(struct tiny_audio_device *adev)
{
    unsigned int i;
    int cur_devices;

    cur_devices = adev->devices;
    ALOGI("Changing devices: 0x%08x", adev->devices);

    if(adev->eq_available)
        vb_effect_sync_devices(cur_devices);

    /* disable old ones. */
    for (i = 0; i < adev->num_dev_cfgs; i++)
	if (!(cur_devices & adev->dev_cfgs[i].mask)) {
#ifdef _VOICE_CALL_VIA_LINEIN
	    if (((AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET | AUDIO_DEVICE_OUT_ALL_FM) ==  adev->dev_cfgs[i].mask)
	        && adev->call_start == 1) {
	        ALOGI("call_start now, off devices is (0x%08x)", cur_devices);
	        continue;
	    }
#endif
	    set_route_by_array(adev->mixer, adev->dev_cfgs[i].off,
			       adev->dev_cfgs[i].off_len);
    }		
		
    /* Turn on new devices */
    for (i = 0; i < adev->num_dev_cfgs; i++)
	if (cur_devices & adev->dev_cfgs[i].mask) {
#ifdef _VOICE_CALL_VIA_LINEIN
	    if (((AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET | AUDIO_DEVICE_OUT_ALL_FM) ==  adev->dev_cfgs[i].mask)
	        && adev->call_start == 1) {
	        ALOGI("call_start now, on devices is (0x%08x)", cur_devices);
	        continue;
	    }
#endif
        ALOGI("Changing devices, mask: 0x%08x", adev->dev_cfgs[i].mask);
        if((cur_devices&AUDIO_DEVICE_OUT_FM_HEADSET) || (cur_devices&AUDIO_DEVICE_OUT_FM_SPEAKER))
        {
               if(adev->dev_cfgs[i].mask & AUDIO_DEVICE_IN_WIRED_HEADSET)
               {
                       ALOGI("do_select_devices fm record don't open main/hp mic (0x%08x)", cur_devices);
                       continue;
               }
	}
	set_route_by_array(adev->mixer, adev->dev_cfgs[i].on,
			       adev->dev_cfgs[i].on_len);
    }

    /* update EQ profile*/

    if(adev->eq_available)
        vb_effect_profile_apply();
#ifndef _VOICE_CALL_VIA_LINEIN
    SetAudio_gain_route(adev,1);
#endif
}

static void select_devices_signal(struct tiny_audio_device *adev)
{
    ALOGI("select_devices_signal starting...");
    sem_post(&adev->routing_mgr.device_switch_sem);
    ALOGI("select_devices_signal finished.");
}

static int start_call(struct tiny_audio_device *adev)
{
    ALOGE("Opening modem PCMs");
#ifdef _VOICE_CALL_VIA_LINEIN
    //open linein function here
    set_call_route(adev, AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET, 1);
#endif
    return 0;
}

static void end_call(struct tiny_audio_device *adev)
{
    ALOGE("Closing modem PCMs");
#ifdef _VOICE_CALL_VIA_LINEIN
    //close linein function.
    set_call_route(adev, AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET, 0);
#endif
}

static void set_eq_filter(struct tiny_audio_device *adev)
{

}

static void set_input_volumes(struct tiny_audio_device *adev, int main_mic_on,
			      int headset_mic_on, int sub_mic_on)
{

}

static void set_output_volumes(struct tiny_audio_device *adev, bool tty_volume)
{
}

static void force_all_standby(struct tiny_audio_device *adev)
{
    struct tiny_stream_in *in;
    struct tiny_stream_out *out;

    if (adev->active_output) {
	out = adev->active_output;
	pthread_mutex_lock(&out->lock);
	do_output_standby(out);
	pthread_mutex_unlock(&out->lock);
    }

    if (adev->active_input) {
	in = adev->active_input;
	pthread_mutex_lock(&in->lock);
	do_input_standby(in);
	pthread_mutex_unlock(&in->lock);
    }
}

static void select_mode(struct tiny_audio_device *adev)
{
    if (adev->mode == AUDIO_MODE_IN_CALL) {
        ALOGE("Entering IN_CALL state, %s first call...devices:0x%x mode:%d ", adev->call_start ? "not":"is",adev->devices,adev->mode);
#ifdef _VOICE_CALL_VIA_LINEIN
        if (!adev->call_start) {
            start_call(adev);
            adev->call_start = 1;
        }
#endif
    } else {
        ALOGE("Leaving IN_CALL state, call_start=%d, mode=%d devices:0x%x ",
	            adev->call_start, adev->mode,adev->devices);
#ifdef _VOICE_CALL_VIA_LINEIN
        if (adev->call_start) {
            end_call(adev);
            adev->call_start = 0;
        }
#endif
    }
}

/* must be called with hw device and output stream mutexes locked */
static int start_output_stream(struct tiny_stream_out *out)
{
    struct tiny_audio_device *adev = out->dev;
    unsigned int card = 0;
    unsigned int port = PORT_MM;
    struct pcm_config old_pcm_config={0};
    int ret=0;

    adev->active_output = out;

    if (!adev->call_start) {
        /* FIXME: only works if only one output can be active at a time */
        select_devices_signal(adev);
    }
    /* default to low power: will be corrected in out_write if necessary before first write to
     * tinyalsa.
     */
    if(adev->call_connected && ( !out->pcm_vplayback)) {
        BLUE_TRACE("open pcm vplayback in");
        card = s_vaudio;
        old_pcm_config=out->config;
        out->config = pcm_config_vplayback;
        out->buffer_vplayback = malloc(RESAMPLER_BUFFER_SIZE);
        out->pcm_vplayback = pcm_open(card, port, PCM_OUT, &out->config);

        if (!pcm_is_ready(out->pcm_vplayback)) {
            out->config = old_pcm_config ;
            ALOGE("cannot open pcm_out driver: %s", pcm_get_error(out->pcm_vplayback));
            pcm_close(out->pcm_vplayback);
            out->pcm_vplayback=NULL;
            free(out->buffer_vplayback);
            out->buffer_vplayback=NULL;
            ALOGE("cannot open pcm_out driver: out\n");
            return 0;
        }
        else {
            ret = create_resampler( DEFAULT_OUT_SAMPLING_RATE,
                                    out->config .rate,
                                    out->config.channels,
                                    RESAMPLER_QUALITY_DEFAULT,
                                    NULL,
                                    &out->resampler_vplayback);
            if (ret != 0) {
                ALOGE("can't  create_resampler");
                pcm_close(out->pcm_vplayback);
                out->pcm_vplayback=NULL;
                free(out->buffer_vplayback);
                out->buffer_vplayback=NULL;
            }
        }
    }
    else {
        BLUE_TRACE("open s_tinycard in");
        card = s_tinycard;
        out->config = pcm_config_mm;
        out->write_threshold = PLAYBACK_LONG_PERIOD_COUNT * LONG_PERIOD_SIZE;
        out->low_power = 1;
        out->config.start_threshold = SHORT_PERIOD_SIZE * PLAYBACK_SHORT_PERIOD_COUNT / 2;
        out->config.avail_min = LONG_PERIOD_SIZE;
        out->pcm = pcm_open(card, port, PCM_OUT | PCM_MMAP | PCM_NOIRQ, &out->config);

        if (!pcm_is_ready(out->pcm)) {
            ALOGE("cannot open pcm_out driver: %s", pcm_get_error(out->pcm));
            pcm_close(out->pcm);
            out->pcm = NULL;
            adev->active_output = NULL;
            return -ENOMEM;
        }
        BLUE_TRACE("open s_tinycard successfully");
    }

    if (adev->echo_reference != NULL)
        out->echo_reference = adev->echo_reference;

    out->resampler->reset(out->resampler);
#ifdef AUDIO_DUMP
    out_dump_create(&out->out_dump_fd, AUDIO_OUT_FILE_PATH);
#endif

    return 0;
}

static int check_input_parameters(uint32_t sample_rate, int format, int channel_count)
{
    if (format != AUDIO_FORMAT_PCM_16_BIT)
        return -EINVAL;

    if ((channel_count < 1) || (channel_count > 2))
        return -EINVAL;

    switch(sample_rate) {
    case 8000:
    case 11025:
    case 16000:
    case 22050:
    case 24000:
    case 32000:
    case 44100:
    case 48000:
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static size_t get_input_buffer_size(uint32_t sample_rate, int format, int channel_count)
{
    size_t size;
    size_t device_rate;

    if (check_input_parameters(sample_rate, format, channel_count) != 0)
        return 0;

    /* take resampling into account and return the closest majoring
    multiple of 16 frames, as audioflinger expects audio buffers to
    be a multiple of 16 frames */
    size = (pcm_config_mm_ul.period_size * sample_rate) / pcm_config_mm_ul.rate;
    size = ((size + 15) / 16) * 16;

    return size * channel_count * sizeof(short);
}

static void add_echo_reference(struct tiny_stream_out *out,
                               struct echo_reference_itfe *reference)
{
    pthread_mutex_lock(&out->lock);
    out->echo_reference = reference;
    pthread_mutex_unlock(&out->lock);
}

static void remove_echo_reference(struct tiny_stream_out *out,
                                  struct echo_reference_itfe *reference)
{
    pthread_mutex_lock(&out->lock);
    if (out->echo_reference == reference) {
        /* stop writing to echo reference */
        reference->write(reference, NULL);
        out->echo_reference = NULL;
    }
    pthread_mutex_unlock(&out->lock);
}

static void put_echo_reference(struct tiny_audio_device *adev,
                          struct echo_reference_itfe *reference)
{
    if (adev->echo_reference != NULL &&
            reference == adev->echo_reference) {
        if (adev->active_output != NULL)
            remove_echo_reference(adev->active_output, reference);
        release_echo_reference(reference);
        adev->echo_reference = NULL;
    }
}

static struct echo_reference_itfe *get_echo_reference(struct tiny_audio_device *adev,
                                               audio_format_t format,
                                               uint32_t channel_count,
                                               uint32_t sampling_rate)
{
    put_echo_reference(adev, adev->echo_reference);
    if (adev->active_output != NULL) {
        struct audio_stream *stream = &adev->active_output->stream.common;
        uint32_t wr_channel_count = popcount(stream->get_channels(stream));
        uint32_t wr_sampling_rate = stream->get_sample_rate(stream);

        int status = create_echo_reference(AUDIO_FORMAT_PCM_16_BIT,
                                           channel_count,
                                           sampling_rate,
                                           AUDIO_FORMAT_PCM_16_BIT,
                                           wr_channel_count,
                                           wr_sampling_rate,
                                           &adev->echo_reference);
        if (status == 0)
            add_echo_reference(adev->active_output, adev->echo_reference);
    }
    return adev->echo_reference;
}

static int get_playback_delay(struct tiny_stream_out *out,
                       size_t frames,
                       struct echo_reference_buffer *buffer)
{
    size_t kernel_frames;
    int status;

    status = pcm_get_htimestamp(out->pcm, &kernel_frames, &buffer->time_stamp);
    if (status < 0) {
        buffer->time_stamp.tv_sec  = 0;
        buffer->time_stamp.tv_nsec = 0;
        buffer->delay_ns           = 0;
        ALOGV("get_playback_delay(): pcm_get_htimestamp error,"
                "setting playbackTimestamp to 0");
        return status;
    }

    kernel_frames = pcm_get_buffer_size(out->pcm) - kernel_frames;

    /* adjust render time stamp with delay added by current driver buffer.
     * Add the duration of current frame as we want the render time of the last
     * sample being written. */
    buffer->delay_ns = (long)(((int64_t)(kernel_frames + frames)* 1000000000)/
                            DEFAULT_OUT_SAMPLING_RATE);

    return 0;
}

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    return DEFAULT_OUT_SAMPLING_RATE;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    return 0;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;

    /* take resampling into account and return the closest majoring
    multiple of 16 frames, as audioflinger expects audio buffers to
    be a multiple of 16 frames */
    size_t size = (SHORT_PERIOD_SIZE * DEFAULT_OUT_SAMPLING_RATE) / out->config.rate;
    size = ((size + 15) / 16) * 16;
    BLUE_TRACE("[TH] size=%d, frame_size=%d", size, audio_stream_frame_size((struct audio_stream *)stream));
    return size * audio_stream_frame_size((struct audio_stream *)stream);
}

static uint32_t out_get_channels(const struct audio_stream *stream)
{
    return AUDIO_CHANNEL_OUT_STEREO;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    return 0;
}

/* must be called with hw device and output stream mutexes locked */
static int do_output_standby(struct tiny_stream_out *out)
{
    struct tiny_audio_device *adev = out->dev;

    if (!out->standby) {
        if (out->pcm) {
            pcm_close(out->pcm);
            out->pcm = NULL;
        }
        BLUE_TRACE("do_output_standby.mode:%d ",adev->mode);
        adev->active_output = 0;

        if(out->pcm_vplayback) {
            pcm_close(out->pcm_vplayback);
            out->pcm_vplayback = NULL;
            if(out->buffer_vplayback) {
                free(out->buffer_vplayback);
                out->buffer_vplayback = 0;
            }
            if(out->resampler_vplayback) {
                release_resampler(out->resampler_vplayback);
                out->resampler_vplayback = 0;
            }
        }
        /* stop writing to echo reference */
        if (out->echo_reference != NULL) {
            out->echo_reference->write(out->echo_reference, NULL);
            out->echo_reference = NULL;
        }
#ifdef AUDIO_DUMP
        out_dump_release(&out->out_dump_fd);
#endif

        out->standby = 1;
    }
    return 0;
}

static int out_standby(struct audio_stream *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    int status;

    pthread_mutex_lock(&out->dev->lock);
    pthread_mutex_lock(&out->lock);
    status = do_output_standby(out);
    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&out->dev->lock);
    return status;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    struct tiny_stream_in *in;
    struct str_parms *parms;
    char *str;
    char value[32];
    int ret, val = 0;
    static int cur_mode = 0;

    BLUE_TRACE("[out_set_parameters], kvpairs=%s devices:0x%x mode:%d ", kvpairs,adev->devices,adev->mode);

    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        ALOGW("[out_set_parameters],after str_parms_get_str,val(0x%x) ",val);
        pthread_mutex_lock(&adev->lock);
        pthread_mutex_lock(&out->lock);
        if ((((adev->devices & AUDIO_DEVICE_OUT_ALL) != val) && (val != 0)) || (AUDIO_MODE_IN_CALL == adev->mode)) {
            adev->devices &= ~AUDIO_DEVICE_OUT_ALL;
            adev->devices |= val;
            ALOGW("out_set_parameters want to set devices:0x%x old_mode:%d new_mode:%d call_start:%d ",adev->devices,cur_mode,adev->mode,adev->call_start);
            cur_mode = adev->mode;
            #ifndef _VOICE_CALL_VIA_LINEIN
            if(!adev->call_start)
            #endif
                select_devices_signal(adev);
            pthread_mutex_unlock(&out->lock);
            pthread_mutex_unlock(&adev->lock);

            if (AUDIO_MODE_IN_CALL == adev->mode) {
                ret = at_cmd_route(adev);  //send at command to cp
                if (ret < 0) {
                    ALOGE("out_set_parameters at_cmd_route error(%d) ",ret);
                    return ret;
                }
            }
        }else{
            pthread_mutex_unlock(&out->lock);
            pthread_mutex_unlock(&adev->lock);
            ALOGW("the same devices(0x%x) with val(0x%x) val is zero...",adev->devices,val);
        }
    }

    ALOGW("out_set_parameters out...call_start:%d",adev->call_start);
    str_parms_destroy(parms);
    return ret;
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;

    return (SHORT_PERIOD_SIZE * PLAYBACK_SHORT_PERIOD_COUNT * 1000) / out->config.rate;
}

static int out_set_volume(struct audio_stream_out *stream, float left,
                          float right)
{
    return -ENOSYS;
}

static bool out_bypass_data(struct tiny_stream_out *out,uint32_t frame_size, uint32_t sample_rate, size_t bytes)
{
    /*
        1. There is some time between call_start and call_connected, we should throw away some data here.
        2. We should throw away some data in BT device during not in a real call.
        3. If mediaserver crash, we should throw away some pcm data after restarting mediaserver.
        4. After call thread gets stop_call cmd, but hasn't get lock.
    */
    struct tiny_audio_device *adev = out->dev;
    int vbc_2arm =  0;
    vbc_2arm = mixer_ctl_get_value(adev->private_ctl.vbc_switch,0);
    if (( (!adev->call_start) && (adev->devices & (AUDIO_DEVICE_OUT_ALL_SCO | AUDIO_DEVICE_OUT_ALL_A2DP)) )
        || (adev->call_start && (!adev->call_connected)) || ((!vbc_2arm) && (!adev->call_start)) || adev->call_prestop) {
        MY_TRACE("out_write throw away data call_start(%d) mode(%d) devices(0x%x) call_connected(%d) vbc_2arm(%d) call_prestop(%d)...",adev->call_start,adev->mode,adev->devices,adev->call_connected,vbc_2arm,adev->call_prestop);
        pthread_mutex_unlock(&adev->lock);
        pthread_mutex_unlock(&out->lock);
        usleep(bytes * 1000000 / frame_size / sample_rate);
        return true;
    }else{
        return false;
    }
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
                         size_t bytes)
{
    int ret;
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    struct tiny_audio_device *adev = out->dev;
    size_t frame_size = 0;
    size_t in_frames = 0;
    size_t out_frames =0;
    struct tiny_stream_in *in;
    bool low_power;
    int kernel_frames;
    void *buf;

    /* acquiring hw device mutex systematically is useful if a low priority thread is waiting
     * on the output stream mutex - e.g. executing select_mode() while holding the hw device
     * mutex
     */

    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&out->lock);
#ifndef _VOICE_CALL_VIA_LINEIN
    if (out_bypass_data(out,audio_stream_frame_size(&stream->common),out_get_sample_rate(&stream->common),bytes)) {
        //release lock
        return bytes;
    }
#endif
    if (out->standby) {
        ret = start_output_stream(out);
        if (ret != 0) {
            pthread_mutex_unlock(&adev->lock);
            goto exit;
        }
        out->standby = 0;
    }
    low_power = adev->low_power && !adev->active_input;
    pthread_mutex_unlock(&adev->lock);

    if (adev->call_connected) {
        BLUE_TRACE("vplayback out_write call_start(%d) call_connected(%d) ...in....",adev->call_start,adev->call_connected);
        frame_size = audio_stream_frame_size(&out->stream.common);
        in_frames = bytes / frame_size;
        out_frames = RESAMPLER_BUFFER_SIZE / frame_size;

        if(out->pcm_vplayback) {
            out->resampler_vplayback->resample_from_input(out->resampler_vplayback,
                                                                (int16_t *)buffer,
                                                                &in_frames,
                                                                (int16_t *)out->buffer_vplayback,
                                                                &out_frames);
            buf = out->buffer_vplayback;
            ret = pcm_write(out->pcm_vplayback, (void *)buf, out_frames*frame_size);
        }
        else
            usleep(out_frames*1000*1000/out->config.rate);

        BLUE_TRACE("vplayback write over result is %d,frame_size is %d in frames %d, out frames %d",ret,frame_size,in_frames,out_frames);
    }
     else {
        frame_size = audio_stream_frame_size(&out->stream.common);
        in_frames = bytes / frame_size;
        out_frames = RESAMPLER_BUFFER_SIZE / frame_size;

        if (low_power != out->low_power) {
            if (low_power) {
                out->write_threshold = LONG_PERIOD_SIZE * PLAYBACK_LONG_PERIOD_COUNT;
                out->config.avail_min = LONG_PERIOD_SIZE;
            } else {
                out->write_threshold = SHORT_PERIOD_SIZE * PLAYBACK_SHORT_PERIOD_COUNT;
                out->config.avail_min = SHORT_PERIOD_SIZE;
            }
            pcm_set_avail_min(out->pcm, out->config.avail_min);
            out->low_power = low_power;
        }

	    /* only use resampler if required */
	    if (out->config.rate != DEFAULT_OUT_SAMPLING_RATE) {
	        out->resampler->resample_from_input(out->resampler,
	                                            (int16_t *)buffer,
	                                            &in_frames,
	                                            (int16_t *)out->buffer,
	                                            &out_frames);
	        buf = out->buffer;
	    } else {
	        out_frames = in_frames;
	        buf = (void *)buffer;
	    }
	    if (out->echo_reference != NULL) {
	        struct echo_reference_buffer b;
	        b.raw = (void *)buffer;
	        b.frame_count = in_frames;

	        get_playback_delay(out, out_frames, &b);
	        out->echo_reference->write(out->echo_reference, &b);
	    }
	    XRUN_TRACE("in_frames=%d, out_frames=%d", in_frames, out_frames);
	    XRUN_TRACE("out->write_threshold=%d, config.avail_min=%d, start_threshold=%d",
	                out->write_threshold,out->config.avail_min, out->config.start_threshold);
	    /* do not allow more than out->write_threshold frames in kernel pcm driver buffer */
	    do {
	        struct timespec time_stamp;

	        if (pcm_get_htimestamp(out->pcm, (unsigned int *)&kernel_frames, &time_stamp) < 0)
	            break;
	        XRUN_TRACE("src_kernel_frames=%d, out_frames=%d", kernel_frames, out_frames);

	        kernel_frames = pcm_get_buffer_size(out->pcm) - kernel_frames;
	        XRUN_TRACE("buffer_size =%d, kernel_frames=%d, wirte_threshold=%d",
	                pcm_get_buffer_size(out->pcm),kernel_frames, out->write_threshold);
	        if (kernel_frames > out->write_threshold) {
	            unsigned long time = (unsigned long)
	                    (((int64_t)(kernel_frames - out->write_threshold) * 1000000) /
	                          DEFAULT_OUT_SAMPLING_RATE);
	            if (time < MIN_WRITE_SLEEP_US)
	                time = MIN_WRITE_SLEEP_US;
	            usleep(time);
	        }
	    } while (kernel_frames > out->write_threshold);

	    ret = pcm_mmap_write(out->pcm, (void *)buf, out_frames * frame_size);
#ifdef AUDIO_DUMP
           out_dump_doing(out->out_dump_fd, (void *)buf, out_frames * frame_size);
#endif
    }

exit:
    if (ret != 0) {
        if (out->pcm)
            ALOGW("warning:%d, (%s)", ret, pcm_get_error(out->pcm));
        else if (out->pcm_vplayback)
            ALOGW("vwarning:%d, (%s)", ret, pcm_get_error(out->pcm_vplayback));
        do_output_standby(out);
    }
    pthread_mutex_unlock(&out->lock);
    return bytes;
}

static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    return 0;
}

/** audio_stream_in implementation **/

/* must be called with hw device and input stream mutexes locked */
static int start_input_stream(struct tiny_stream_in *in)
{
    int ret = 0;
    struct tiny_audio_device *adev = in->dev;

    adev->active_input = in;
    ALOGW("start_input_stream in mode:0x%x devices:0x%x call_start:%d ",adev->mode,adev->devices,adev->call_start);
    if (!adev->call_start) {
        adev->devices &= ~AUDIO_DEVICE_IN_ALL;
        adev->devices |= in->device;
        select_devices_signal(adev);
    }

    if (in->need_echo_reference && in->echo_reference == NULL)
        in->echo_reference = get_echo_reference(adev,
                                        AUDIO_FORMAT_PCM_16_BIT,
                                        in->config.channels,
                                        in->requested_rate);
    BLUE_TRACE("[TH], start_input,channels=%d,peroid_size=%d, peroid_count=%d,rate=%d",
                in->config.channels, in->config.period_size,
                in->config.period_count, in->config.rate);
    /* this assumes routing is done previously */

    if(adev->call_start) {
        in->active_rec_proc = 0;
        in->pcm = pcm_open(s_vaudio,PORT_MM,PCM_IN,&pcm_config_vrec_vx);
        if (!pcm_is_ready(in->pcm)) {
            ALOGE("voice-call rec cannot open pcm_in driver: %s", pcm_get_error(in->pcm));
            pcm_close(in->pcm);
            in->pcm = NULL;
            adev->active_input = NULL;
            return -ENOMEM;
        }
    } else {
        in->pcm = pcm_open(s_tinycard, PORT_MM, PCM_IN, &in->config);
        if (!pcm_is_ready(in->pcm)) {
            ALOGE("normal rec cannot open pcm_in driver: %s", pcm_get_error(in->pcm));
            pcm_close(in->pcm);
            in->pcm = NULL;
            adev->active_input = NULL;
            return -ENOMEM;
        }
        /* start to process pcm data captured, such as noise suppression.*/
        in->active_rec_proc = init_rec_process(get_mode_from_devices(in->device), in->config.rate);
        ALOGI("record process module created is %s.", in->active_rec_proc ? "successful" : "failed");
    }
    /* if no supported sample rate is available, use the resampler */
    if (in->resampler) {
        in->resampler->reset(in->resampler);
        in->frames_in = 0;
    }
    return 0;
}

static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;

    return in->requested_rate;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    return 0;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    size_t size;
    size_t device_rate;

    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;

    if (check_input_parameters(in->requested_rate, AUDIO_FORMAT_PCM_16_BIT, in->config.channels) != 0)
        return 0;

    /* take resampling into account and return the closest majoring
    multiple of 16 frames, as audioflinger expects audio buffers to
    be a multiple of 16 frames */
    size = (in->config.period_size * in->requested_rate) / in->config.rate;
    size = ((size + 15) / 16) * 16;

    return size * in->config.channels * sizeof(short);
}

static uint32_t in_get_channels(const struct audio_stream *stream)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;

    if (in->config.channels == 1) {
        return AUDIO_CHANNEL_IN_MONO;
    } else {
        return AUDIO_CHANNEL_IN_STEREO;
    }
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    return 0;
}

/* must be called with hw device and input stream mutexes locked */
static int do_input_standby(struct tiny_stream_in *in)
{
    struct tiny_audio_device *adev = in->dev;

    if (!in->standby) {
        if (in->pcm) {
            pcm_close(in->pcm);
            in->pcm = NULL;
        }
        adev->active_input = 0;
        if (adev->mode != AUDIO_MODE_IN_CALL) {
            adev->devices &= ~AUDIO_DEVICE_IN_ALL;
            select_devices_signal(adev);
        }

        if (in->echo_reference != NULL) {
            /* stop reading from echo reference */
            in->echo_reference->read(in->echo_reference, NULL);
            put_echo_reference(adev, in->echo_reference);
            in->echo_reference = NULL;
        }
        if (in->active_rec_proc) {
            AUDPROC_DeInitDp();
            in->active_rec_proc = 0;
        }
        in->standby = 1;
    }
    return 0;
}

static int in_standby(struct audio_stream *stream)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    int status;

    pthread_mutex_lock(&in->dev->lock);
    pthread_mutex_lock(&in->lock);
    status = do_input_standby(in);
    pthread_mutex_unlock(&in->lock);
    pthread_mutex_unlock(&in->dev->lock);
    return status;
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    struct tiny_audio_device *adev = in->dev;
    struct str_parms *parms;
    char *str;
    char value[32];
    int ret, val = 0;

    BLUE_TRACE("[in_set_parameters], kvpairs=%s devices:0x%x mode:%d ", kvpairs,adev->devices,adev->mode);

    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_INPUT_SOURCE, value, sizeof(value));

    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&in->lock);
    if (ret >= 0) {
        val = atoi(value);
        /* no audio source uses val == 0 */
        if ((in->source != val) && (val != 0)) {
            in->source = val;
        }
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        if ((in->device != val) && (val != 0)) {
            in->device = val;
            adev->devices &= ~AUDIO_DEVICE_IN_ALL;
            adev->devices |= in->device;
            select_devices_signal(adev);
        }
    }

    pthread_mutex_unlock(&adev->lock);
    pthread_mutex_unlock(&in->lock);

    str_parms_destroy(parms);
    return ret;
}

static char * in_get_parameters(const struct audio_stream *stream,
                                const char *keys)
{
    return strdup("");
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    return 0;
}

static void get_capture_delay(struct tiny_stream_in *in,
                       size_t frames,
                       struct echo_reference_buffer *buffer)
{

    /* read frames available in kernel driver buffer */
    size_t kernel_frames;
    struct timespec tstamp;
    long buf_delay;
    long rsmp_delay;
    long kernel_delay;
    long delay_ns;

    if (pcm_get_htimestamp(in->pcm, &kernel_frames, &tstamp) < 0) {
        buffer->time_stamp.tv_sec  = 0;
        buffer->time_stamp.tv_nsec = 0;
        buffer->delay_ns           = 0;
        ALOGW("read get_capture_delay(): pcm_htimestamp error");
        return;
    }

    /* read frames available in audio HAL input buffer
     * add number of frames being read as we want the capture time of first sample
     * in current buffer */
    buf_delay = (long)(((int64_t)(in->frames_in + in->proc_frames_in) * 1000000000)
                                    / in->config.rate);
    /* add delay introduced by resampler */
    rsmp_delay = 0;
    if (in->resampler) {
        rsmp_delay = in->resampler->delay_ns(in->resampler);
    }

    kernel_delay = (long)(((int64_t)kernel_frames * 1000000000) / in->config.rate);

    delay_ns = kernel_delay + buf_delay + rsmp_delay;

    buffer->time_stamp = tstamp;
    buffer->delay_ns   = delay_ns;
    ALOGV("get_capture_delay time_stamp = [%ld].[%ld], delay_ns: [%d],"
         " kernel_delay:[%ld], buf_delay:[%ld], rsmp_delay:[%ld], kernel_frames:[%d], "
         "in->frames_in:[%d], in->proc_frames_in:[%d], frames:[%d]",
         buffer->time_stamp.tv_sec , buffer->time_stamp.tv_nsec, buffer->delay_ns,
         kernel_delay, buf_delay, rsmp_delay, kernel_frames,
         in->frames_in, in->proc_frames_in, frames);

}

static int32_t update_echo_reference(struct tiny_stream_in *in, size_t frames)
{
    struct echo_reference_buffer b;
    b.delay_ns = 0;

    ALOGV("update_echo_reference, frames = [%d], in->ref_frames_in = [%d],  "
          "b.frame_count = [%d]",
         frames, in->ref_frames_in, frames - in->ref_frames_in);
    if (in->ref_frames_in < frames) {
        if (in->ref_buf_size < frames) {
            in->ref_buf_size = frames;
            in->ref_buf = (int16_t *)realloc(in->ref_buf,
                                             in->ref_buf_size *
                                                 in->config.channels * sizeof(int16_t));
        }

        b.frame_count = frames - in->ref_frames_in;
        b.raw = (void *)(in->ref_buf + in->ref_frames_in * in->config.channels);

        get_capture_delay(in, frames, &b);

        if (in->echo_reference->read(in->echo_reference, &b) == 0)
        {
            in->ref_frames_in += b.frame_count;
            ALOGV("update_echo_reference: in->ref_frames_in:[%d], "
                    "in->ref_buf_size:[%d], frames:[%d], b.frame_count:[%d]",
                 in->ref_frames_in, in->ref_buf_size, frames, b.frame_count);
        }
    } else
        ALOGW("update_echo_reference: NOT enough frames to read ref buffer");
    return b.delay_ns;
}

static int set_preprocessor_param(effect_handle_t handle,
                           effect_param_t *param)
{
    uint32_t size = sizeof(int);
    uint32_t psize = ((param->psize - 1) / sizeof(int) + 1) * sizeof(int) +
                        param->vsize;

    int status = (*handle)->command(handle,
                                   EFFECT_CMD_SET_PARAM,
                                   sizeof (effect_param_t) + psize,
                                   param,
                                   &size,
                                   &param->status);
    if (status == 0)
        status = param->status;

    return status;
}

static int set_preprocessor_echo_delay(effect_handle_t handle,
                                     int32_t delay_us)
{
    uint32_t buf[sizeof(effect_param_t) / sizeof(uint32_t) + 2];
    effect_param_t *param = (effect_param_t *)buf;

    param->psize = sizeof(uint32_t);
    param->vsize = sizeof(uint32_t);
    *(uint32_t *)param->data = AEC_PARAM_ECHO_DELAY;
    *((int32_t *)param->data + 1) = delay_us;

    return set_preprocessor_param(handle, param);
}

static void push_echo_reference(struct tiny_stream_in *in, size_t frames)
{
    /* read frames from echo reference buffer and update echo delay
     * in->ref_frames_in is updated with frames available in in->ref_buf */
    int32_t delay_us = update_echo_reference(in, frames)/1000;
    int i;
    audio_buffer_t buf;

    if (in->ref_frames_in < frames)
        frames = in->ref_frames_in;

    buf.frameCount = frames;
    buf.raw = in->ref_buf;

    for (i = 0; i < in->num_preprocessors; i++) {
        if ((*in->preprocessors[i])->process_reverse == NULL)
            continue;

        (*in->preprocessors[i])->process_reverse(in->preprocessors[i],
                                               &buf,
                                               NULL);
        set_preprocessor_echo_delay(in->preprocessors[i], delay_us);
    }

    in->ref_frames_in -= buf.frameCount;
    if (in->ref_frames_in) {
        memcpy(in->ref_buf,
               in->ref_buf + buf.frameCount * in->config.channels,
               in->ref_frames_in * in->config.channels * sizeof(int16_t));
    }
}

static int get_next_buffer(struct resampler_buffer_provider *buffer_provider,
                                   struct resampler_buffer* buffer)
{
    struct tiny_stream_in *in;

    if (buffer_provider == NULL || buffer == NULL)
        return -EINVAL;

    in = (struct tiny_stream_in *)((char *)buffer_provider -
                                   offsetof(struct tiny_stream_in, buf_provider));

    if (in->pcm == NULL) {
        buffer->raw = NULL;
        buffer->frame_count = 0;
        in->read_status = -ENODEV;
        return -ENODEV;
    }

    if (in->frames_in == 0) {
        in->read_status = pcm_read(in->pcm,
                                   (void*)in->buffer,
                                   in->config.period_size *
                                       audio_stream_frame_size(&in->stream.common));
        if (in->read_status != 0) {
            ALOGE("get_next_buffer() pcm_read sattus=%d, error: %s",
                                in->read_status, pcm_get_error(in->pcm));
            buffer->raw = NULL;
            buffer->frame_count = 0;
            return in->read_status;
        }
        if (in->active_rec_proc)
            aud_rec_do_process((void *)in->buffer,
                                in->config.period_size *audio_stream_frame_size(&in->stream.common));
        in->frames_in = in->config.period_size;
    }

    buffer->frame_count = (buffer->frame_count > in->frames_in) ?
                                in->frames_in : buffer->frame_count;
    buffer->i16 = in->buffer + (in->config.period_size - in->frames_in) *
                                                in->config.channels;

    return in->read_status;

}

static void release_buffer(struct resampler_buffer_provider *buffer_provider,
                                  struct resampler_buffer* buffer)
{
    struct tiny_stream_in *in;

    if (buffer_provider == NULL || buffer == NULL)
        return;

    in = (struct tiny_stream_in *)((char *)buffer_provider -
                                   offsetof(struct tiny_stream_in, buf_provider));

    in->frames_in -= buffer->frame_count;
}

/* read_frames() reads frames from kernel driver, down samples to capture rate
 * if necessary and output the number of frames requested to the buffer specified */
static ssize_t read_frames(struct tiny_stream_in *in, void *buffer, ssize_t frames)
{
    ssize_t frames_wr = 0;
//BLUE_TRACE("read_frames, frames=%d", frames);
    while (frames_wr < frames) {
        size_t frames_rd = frames - frames_wr;
//BLUE_TRACE("frames_wr=%d, frames=%d, frames_rd=%d", frames_wr, frames, frames_rd);
        if (in->resampler != NULL) {
            in->resampler->resample_from_provider(in->resampler,
                    (int16_t *)((char *)buffer +
                            frames_wr * audio_stream_frame_size(&in->stream.common)),
                    &frames_rd);
        } else {
            struct resampler_buffer buf = {
                    { raw : NULL, },
                    frame_count : frames_rd,
            };
            get_next_buffer(&in->buf_provider, &buf);
            if (buf.raw != NULL) {
                memcpy((char *)buffer +
                           frames_wr * audio_stream_frame_size(&in->stream.common),
                        buf.raw,
                        buf.frame_count * audio_stream_frame_size(&in->stream.common));
                frames_rd = buf.frame_count;
            }
            release_buffer(&in->buf_provider, &buf);
        }
        /* in->read_status is updated by getNextBuffer() also called by
         * in->resampler->resample_from_provider() */
        if (in->read_status != 0)
            return in->read_status;

        frames_wr += frames_rd;
    }
    return frames_wr;
}

/* process_frames() reads frames from kernel driver (via read_frames()),
 * calls the active audio pre processings and output the number of frames requested
 * to the buffer specified */
static ssize_t process_frames(struct tiny_stream_in *in, void* buffer, ssize_t frames)
{
    ssize_t frames_wr = 0;
    audio_buffer_t in_buf;
    audio_buffer_t out_buf;
    int i;

    while (frames_wr < frames) {
        /* first reload enough frames at the end of process input buffer */
        if (in->proc_frames_in < (size_t)frames) {
            ssize_t frames_rd;

            if (in->proc_buf_size < (size_t)frames) {
                in->proc_buf_size = (size_t)frames;
                in->proc_buf = (int16_t *)realloc(in->proc_buf,
                                         in->proc_buf_size *
                                             in->config.channels * sizeof(int16_t));
                ALOGV("process_frames(): in->proc_buf %p size extended to %d frames",
                     in->proc_buf, in->proc_buf_size);
            }
            frames_rd = read_frames(in,
                                    in->proc_buf +
                                        in->proc_frames_in * in->config.channels,
                                    frames - in->proc_frames_in);
            if (frames_rd < 0) {
                frames_wr = frames_rd;
                break;
            }
            in->proc_frames_in += frames_rd;
        }

        if (in->echo_reference != NULL)
            push_echo_reference(in, in->proc_frames_in);

         /* in_buf.frameCount and out_buf.frameCount indicate respectively
          * the maximum number of frames to be consumed and produced by process() */
        in_buf.frameCount = in->proc_frames_in;
        in_buf.s16 = in->proc_buf;
        out_buf.frameCount = frames - frames_wr;
        out_buf.s16 = (int16_t *)buffer + frames_wr * in->config.channels;

        for (i = 0; i < in->num_preprocessors; i++)
            (*in->preprocessors[i])->process(in->preprocessors[i],
                                               &in_buf,
                                               &out_buf);

        /* process() has updated the number of frames consumed and produced in
         * in_buf.frameCount and out_buf.frameCount respectively
         * move remaining frames to the beginning of in->proc_buf */
        in->proc_frames_in -= in_buf.frameCount;
        if (in->proc_frames_in) {
            memcpy(in->proc_buf,
                   in->proc_buf + in_buf.frameCount * in->config.channels,
                   in->proc_frames_in * in->config.channels * sizeof(int16_t));
        }

        /* if not enough frames were passed to process(), read more and retry. */
        if (out_buf.frameCount == 0)
            continue;

        frames_wr += out_buf.frameCount;
    }
    return frames_wr;
}

static bool in_bypass_data(struct tiny_stream_in *in,uint32_t frame_size, uint32_t sample_rate, void* buffer, size_t bytes)
{
    struct tiny_audio_device *adev = in->dev;
    /*
        1. If cp stopped calling and in-devices is AUDIO_DEVICE_IN_VOICE_CALL, it means that cp already stopped vt call, we should write
            0 data, otherwise, AudioRecord will obtainbuffer timeout.
    */
   if ((!adev->call_start) && (adev->mode == AUDIO_MODE_IN_CALL) && ((in->device == AUDIO_DEVICE_IN_VOICE_CALL))){
       ALOGW("in_bypass_data write 0 data call_start(%d) mode(%d) devices(0x%x) in_device(0x%x) call_connected(%d) call_prestop(%d) ",adev->call_start,adev->mode,adev->devices,in->device,adev->call_connected,adev->call_prestop);
       pthread_mutex_unlock(&adev->lock);
       pthread_mutex_unlock(&in->lock);
       memset(buffer,0,bytes);
       usleep(bytes * 1000000 / frame_size / sample_rate);
       return true;
   }else{
     return false;
   }
}

static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
                       size_t bytes)
{
    int ret = 0;
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    struct tiny_audio_device *adev = in->dev;
    size_t frames_rq = bytes / audio_stream_frame_size(&stream->common);

    /* acquiring hw device mutex systematically is useful if a low priority thread is waiting
     * on the input stream mutex - e.g. executing select_mode() while holding the hw device
     * mutex
     */
    pthread_mutex_lock(&adev->lock);
    pthread_mutex_lock(&in->lock);
#ifndef _VOICE_CALL_VIA_LINEIN
    if(in_bypass_data(in,audio_stream_frame_size(&stream->common),in_get_sample_rate(&stream->common),buffer,bytes)){
        //release lock
        return bytes;
    }
#endif
    if (in->standby) {
        ret = start_input_stream(in);
        if (ret == 0)
            in->standby = 0;
    }
    pthread_mutex_unlock(&adev->lock);

    if (ret < 0)
        goto exit;
    /*BLUE_TRACE("in_read start.num_preprocessors=%d, resampler=%d",
                    in->num_preprocessors, in->resampler);*/
    if (in->num_preprocessors != 0)
        ret = process_frames(in, buffer, frames_rq);
    else if (in->resampler != NULL)
        ret = read_frames(in, buffer, frames_rq);
    else {
        ret = pcm_read(in->pcm, buffer, bytes);
        if (ret == 0 && in->active_rec_proc)
            aud_rec_do_process(buffer, bytes);
    }

    if (ret > 0)
        ret = 0;

    if (ret == 0 && adev->mic_mute)
        memset(buffer, 0, bytes);
    /*BLUE_TRACE("in_read OK, bytes=%d", bytes);*/
exit:
    if (ret < 0) {
        ALOGW("in_read,warning: ret=%d, (%s)", ret, pcm_get_error(in->pcm));
        do_input_standby(in);
    }
    pthread_mutex_unlock(&in->lock);
    return bytes;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    return 0;
}

static int in_add_audio_effect(const struct audio_stream *stream,
                               effect_handle_t effect)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    int status;
    effect_descriptor_t desc;

    pthread_mutex_lock(&in->dev->lock);
    pthread_mutex_lock(&in->lock);
    if (in->num_preprocessors >= MAX_PREPROCESSORS) {
        status = -ENOSYS;
        goto exit;
    }

    status = (*effect)->get_descriptor(effect, &desc);
    if (status != 0)
        goto exit;

    in->preprocessors[in->num_preprocessors++] = effect;

    if (memcmp(&desc.type, FX_IID_AEC, sizeof(effect_uuid_t)) == 0) {
        in->need_echo_reference = true;
        do_input_standby(in);
    }

exit:

    pthread_mutex_unlock(&in->lock);
    pthread_mutex_unlock(&in->dev->lock);
    return status;
}

static int in_remove_audio_effect(const struct audio_stream *stream,
                                  effect_handle_t effect)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;
    int i;
    int status = -EINVAL;
    bool found = false;
    effect_descriptor_t desc;

    pthread_mutex_lock(&in->dev->lock);
    pthread_mutex_lock(&in->lock);
    if (in->num_preprocessors <= 0) {
        status = -ENOSYS;
        goto exit;
    }

    for (i = 0; i < in->num_preprocessors; i++) {
        if (found) {
            in->preprocessors[i - 1] = in->preprocessors[i];
            continue;
        }
        if (in->preprocessors[i] == effect) {
            in->preprocessors[i] = NULL;
            status = 0;
            found = true;
        }
    }

    if (status != 0)
        goto exit;

    in->num_preprocessors--;

    status = (*effect)->get_descriptor(effect, &desc);
    if (status != 0)
        goto exit;
    if (memcmp(&desc.type, FX_IID_AEC, sizeof(effect_uuid_t)) == 0) {
        in->need_echo_reference = false;
        do_input_standby(in);
    }

exit:

    pthread_mutex_unlock(&in->lock);
    pthread_mutex_unlock(&in->dev->lock);
    return status;
}

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   uint32_t devices, int *format,
                                   uint32_t *channels, uint32_t *sample_rate,
                                   struct audio_stream_out **stream_out)
{
    struct tiny_audio_device *ladev = (struct tiny_audio_device *)dev;
    struct tiny_stream_out *out;
    int ret;

    BLUE_TRACE("adev_open_output_stream, devices=%d", devices);
    out = (struct tiny_stream_out *)calloc(1, sizeof(struct tiny_stream_out));
    if (!out)
        return -ENOMEM;
    memset(out, 0, sizeof(struct tiny_stream_out));
    ret = create_resampler(DEFAULT_OUT_SAMPLING_RATE,
                           MM_FULL_POWER_SAMPLING_RATE,
                           2,
                           RESAMPLER_QUALITY_DEFAULT,
                           NULL,
                           &out->resampler);
    if (ret != 0)
        goto err_open;
    out->buffer = malloc(RESAMPLER_BUFFER_SIZE); /* todo: allow for reallocing */

    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.get_channels = out_get_channels;
    out->stream.common.get_format = out_get_format;
    out->stream.common.set_format = out_set_format;
    out->stream.common.standby = out_standby;
    out->stream.common.dump = out_dump;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.write = out_write;
    out->stream.get_render_position = out_get_render_position;

    out->config = pcm_config_mm;

    out->dev = ladev;
    out->standby = 1;

    /* FIXME: when we support multiple output devices, we will want to
     * do the following:
     * adev->devices &= ~AUDIO_DEVICE_OUT_ALL;
     * adev->devices |= out->device;
     * select_output_device(adev);
     * This is because out_set_parameters() with a route is not
     * guaranteed to be called after an output stream is opened. */

    *format = out_get_format(&out->stream.common);
    *channels = out_get_channels(&out->stream.common);
    *sample_rate = out_get_sample_rate(&out->stream.common);

    BLUE_TRACE("Successful, adev_open_output_stream");
    *stream_out = &out->stream;
    return 0;

err_open:
    BLUE_TRACE("Error adev_open_output_stream");
    free(out);
    *stream_out = NULL;
    return ret;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    struct tiny_stream_out *out = (struct tiny_stream_out *)stream;
    BLUE_TRACE("adev_close_output_stream");
    out_standby(&stream->common);
    if (out->buffer)
        free(out->buffer);
    if (out->resampler)
        release_resampler(out->resampler);

    if(out->buffer_vplayback)
        free(out->buffer_vplayback);
    if(out->resampler_vplayback)
        release_resampler(out->resampler_vplayback);
    free(stream);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    struct str_parms *parms;
    char *str;
    char value[32];
    int ret;

    BLUE_TRACE("adev_set_parameters, kvpairs : %s", kvpairs);
    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_BT_NREC, value, sizeof(value));
    if (ret >= 0) {
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0)
            adev->bluetooth_nrec = true;
        else
            adev->bluetooth_nrec = false;
    }

    ret = str_parms_get_str(parms, "screen_state", value, sizeof(value));
    if (ret >= 0) {
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0)
            adev->low_power = false;
        else
            adev->low_power = true;
    }

    str_parms_destroy(parms);
    return ret;
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
                                  const char *keys)
{
    return strdup("");
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    ALOGW("adev_init_check");
    return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    BLUE_TRACE("adev_set_voice_volume in...volume:%f mode:%d call_start:%d ",volume,adev->mode,adev->call_start);

    adev->voice_volume = volume;
    /*Send at command to cp side*/
    at_cmd_volume(volume,adev->mode);

    return 0;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    ALOGW("adev_set_master_volume in...devices:0x%x ,volume:%f ",adev->devices,volume);
    #ifndef _VOICE_CALL_VIA_LINEIN
    SetAudio_gain_route(adev,1);
    #endif
    return -ENOSYS;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;
    BLUE_TRACE("adev_set_mode, mode=%d", mode);
    pthread_mutex_lock(&adev->lock);
    if (adev->mode != mode) {
        adev->mode = mode;
        select_mode(adev);
    }else{
    	BLUE_TRACE("adev_set_mode,the same mode(%d)",mode);
    }
    pthread_mutex_unlock(&adev->lock);

    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;

    adev->mic_mute = state;

    return 0;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    struct tiny_audio_device *adev = (struct tiny_audio_device *)dev;

    *state = adev->mic_mute;

    return 0;
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
                                         uint32_t sample_rate, int format,
                                         int channel_count)
{
    size_t size;

    if (check_input_parameters(sample_rate, format, channel_count) != 0)
        return 0;

    return get_input_buffer_size(sample_rate, format, channel_count);
}

static int adev_open_input_stream(struct audio_hw_device *dev, uint32_t devices,
                                  int *format, uint32_t *channel_mask,
                                  uint32_t *sample_rate,
                                  audio_in_acoustics_t acoustics,
                                  struct audio_stream_in **stream_in)

{
    struct tiny_audio_device *ladev = (struct tiny_audio_device *)dev;
    struct tiny_stream_in *in;
    int ret;
    int channel_count = popcount(*channel_mask);

    BLUE_TRACE("[TH], adev_open_input_stream,devices=0x%x,sample_rate=%d, channel_count=%d",
                           devices, *sample_rate, channel_count);

    if (check_input_parameters(*sample_rate, *format, channel_count) != 0)
        return -EINVAL;

    in = (struct tiny_stream_in *)calloc(1, sizeof(struct tiny_stream_in));
    if (!in)
        return -ENOMEM;
    memset(in, 0, sizeof(struct tiny_stream_in));
    in->stream.common.get_sample_rate = in_get_sample_rate;
    in->stream.common.set_sample_rate = in_set_sample_rate;
    in->stream.common.get_buffer_size = in_get_buffer_size;
    in->stream.common.get_channels = in_get_channels;
    in->stream.common.get_format = in_get_format;
    in->stream.common.set_format = in_set_format;
    in->stream.common.standby = in_standby;
    in->stream.common.dump = in_dump;
    in->stream.common.set_parameters = in_set_parameters;
    in->stream.common.get_parameters = in_get_parameters;
    in->stream.common.add_audio_effect = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;
    in->stream.set_gain = in_set_gain;
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;

    in->requested_rate = *sample_rate;
#ifndef _VOICE_CALL_VIA_LINEIN
    if (ladev->call_start)
        memcpy(&in->config, &pcm_config_vrec_vx, sizeof(pcm_config_vrec_vx));
    else
#endif
        memcpy(&in->config, &pcm_config_mm_ul, sizeof(pcm_config_mm_ul));
    in->config.channels = channel_count;

    if (in->requested_rate != in->config.rate) {
        in->buf_provider.get_next_buffer = get_next_buffer;
        in->buf_provider.release_buffer = release_buffer;

        in->buffer = malloc(in->config.period_size *
                audio_stream_frame_size(&in->stream.common));
        if (!in->buffer) {
            ret = -ENOMEM;
            goto err;
        }

        ret = create_resampler(in->config.rate,
                               in->requested_rate,
                               in->config.channels,
                               RESAMPLER_QUALITY_DEFAULT,
                               &in->buf_provider,
                               &in->resampler);
        if (ret != 0) {
            ret = -EINVAL;
            goto err;
        }
    }

    in->dev = ladev;
    in->standby = 1;
    in->device = devices;

    *stream_in = &in->stream;
    BLUE_TRACE("Successfully, adev_open_input_stream.");
    return 0;

err:
    BLUE_TRACE("Failed(%d), adev_open_input_stream.", ret);
    if (in->buffer)
        free(in->buffer);
    if (in->resampler)
        release_resampler(in->resampler);

    free(in);
    *stream_in = NULL;
    return ret;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                                   struct audio_stream_in *stream)
{
    struct tiny_stream_in *in = (struct tiny_stream_in *)stream;

    in_standby(&stream->common);

    if (in->resampler) {
        free(in->buffer);
        release_resampler(in->resampler);
    }
    if (in->proc_buf)
        free(in->proc_buf);
    if (in->ref_buf)
        free(in->ref_buf);

    free(stream);
    return;
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    return 0;
}

static int adev_close(hw_device_t *device)
{
    unsigned int i, j;
    struct tiny_audio_device *adev = (struct tiny_audio_device *)device;
    /* free audio PGA */
    audio_pga_free(adev->pga);
#ifndef _VOICE_CALL_VIA_LINEIN
    vbc_ctrl_close();
#endif
    //Need to free mixer configs here.
    for (i=0; i < adev->num_dev_cfgs; i++) {
        for (j=0; j < adev->dev_cfgs->on_len; j++) {
            free(adev->dev_cfgs[i].on[j].ctl_name);
            //Is there a string of strval?
        };
        free(adev->dev_cfgs[i].on);
        for (j=0; j < adev->dev_cfgs->off_len; j++) {
            free(adev->dev_cfgs[i].off[j].ctl_name);
        };
        free(adev->dev_cfgs[i].off);
    };
    free(adev->dev_cfgs);

    mixer_close(adev->mixer);
    stream_routing_manager_close(adev);
    free(device);
    return 0;
}

static uint32_t adev_get_supported_devices(const struct audio_hw_device *dev)
{
    return (/* OUT */
            AUDIO_DEVICE_OUT_EARPIECE |
            AUDIO_DEVICE_OUT_SPEAKER |
            AUDIO_DEVICE_OUT_WIRED_HEADSET |
            AUDIO_DEVICE_OUT_WIRED_HEADPHONE |
            AUDIO_DEVICE_OUT_AUX_DIGITAL |
            AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET |
            AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET |
            AUDIO_DEVICE_OUT_ALL_SCO |
            AUDIO_DEVICE_OUT_ALL_FM |
            AUDIO_DEVICE_OUT_DEFAULT |
            /* IN */
            AUDIO_DEVICE_IN_COMMUNICATION |
            AUDIO_DEVICE_IN_AMBIENT |
            AUDIO_DEVICE_IN_BUILTIN_MIC |
            AUDIO_DEVICE_IN_WIRED_HEADSET |
            AUDIO_DEVICE_IN_AUX_DIGITAL |
            AUDIO_DEVICE_IN_BACK_MIC |
            AUDIO_DEVICE_IN_ALL_SCO |
            AUDIO_DEVICE_IN_VOICE_CALL |
            AUDIO_DEVICE_IN_DEFAULT);
}

/* parse the private field of xml config file. */
static void adev_config_parse_private(struct config_parse_state *s, const XML_Char *name)
{
    if (s && name) {
        if (strcmp(s->private_name, PRIVATE_VBC_CONTROL) == 0) {
            s->adev->private_ctl.vbc_switch =
                 mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.vbc_switch);
        } else if (strcmp(s->private_name, PRIVATE_VBC_EQ_SWITCH) == 0) {
            s->adev->private_ctl.vbc_eq_switch =
                 mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.vbc_eq_switch);
        } else if (strcmp(s->private_name, PRIVATE_VBC_EQ_UPDATE) == 0) {
            s->adev->private_ctl.vbc_eq_update =
                 mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.vbc_eq_update);
        } else if (strcmp(s->private_name, PRIVATE_VBC_EQ_PROFILE) == 0) {
            s->adev->private_ctl.vbc_eq_profile_select =
                 mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.vbc_eq_profile_select);
        } else if (strcmp(s->private_name, PRIVATE_MIC_BIAS) == 0) {
            s->adev->private_ctl.mic_bias_switch =
                 mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.mic_bias_switch);
        } else if (strcmp(s->private_name, PRIVATE_INTERNAL_PA) == 0) {
            s->adev->private_ctl.internal_pa =
                 mixer_get_ctl_by_name(s->adev->mixer, name);
            CTL_TRACE(s->adev->private_ctl.internal_pa);
        }
    }
}

static void adev_config_start(void *data, const XML_Char *elem,
			      const XML_Char **attr)
{
    struct config_parse_state *s = data;
    struct tiny_dev_cfg *dev_cfg;
    const XML_Char *name = NULL;
    const XML_Char *val = NULL;
    unsigned int i, j;

    for (i = 0; attr[i]; i += 2) {
	if (strcmp(attr[i], "name") == 0)
	    name = attr[i + 1];

	if (strcmp(attr[i], "val") == 0)
	    val = attr[i + 1];
    }

    if (strcmp(elem, "device") == 0) {
	if (!name) {
	    ALOGE("Unnamed device\n");
	    return;
	}

	for (i = 0; i < sizeof(dev_names) / sizeof(dev_names[0]); i++) {
	    if (strcmp(dev_names[i].name, name) == 0) {
		ALOGI("Allocating device %s\n", name);
		dev_cfg = realloc(s->adev->dev_cfgs,
				  (s->adev->num_dev_cfgs + 1)
				  * sizeof(*dev_cfg));
		if (!dev_cfg) {
		    ALOGE("Unable to allocate dev_cfg\n");
		    return;
		}

		s->dev = &dev_cfg[s->adev->num_dev_cfgs];
		memset(s->dev, 0, sizeof(*s->dev));
		s->dev->mask = dev_names[i].mask;

		s->adev->dev_cfgs = dev_cfg;
		s->adev->num_dev_cfgs++;
	    }
	}

    } else if (strcmp(elem, "path") == 0) {
	if (s->path_len)
	    ALOGW("Nested paths\n");

	/* If this a path for a device it must have a role */
	if (s->dev) {
	    /* Need to refactor a bit... */
	    if (strcmp(name, "on") == 0) {
		s->on = true;
	    } else if (strcmp(name, "off") == 0) {
		s->on = false;
	    } else {
		ALOGW("Unknown path name %s\n", name);
	    }
	}

    } else if (strcmp(elem, "ctl") == 0) {
	struct route_setting *r;

	if (!name) {
	    ALOGE("Unnamed control\n");
	    return;
	}

	if (!val) {
	    ALOGE("No value specified for %s\n", name);
	    return;
	}

	ALOGI("Parsing control %s => %s\n", name, val);

	r = realloc(s->path, sizeof(*r) * (s->path_len + 1));
	if (!r) {
	    ALOGE("Out of memory handling %s => %s\n", name, val);
	    return;
	}

	r[s->path_len].ctl_name = strdup(name);
	r[s->path_len].strval = NULL;

	/* This can be fooled but it'll do */
	r[s->path_len].intval = atoi(val);
	if (!r[s->path_len].intval && strcmp(val, "0") != 0)
	    r[s->path_len].strval = strdup(val);

	s->path = r;
	s->path_len++;
        ALOGI("s->path_len=%d", s->path_len);
    }
    else if (strcmp(elem, "private") == 0) {
        memset(s->private_name, 0, PRIVATE_NAME_LEN);
        memcpy(s->private_name, name, strlen(name));
    }
    else if (strcmp(elem, "func") == 0) {
        adev_config_parse_private(s, name);
    }
}

static void adev_config_end(void *data, const XML_Char *name)
{
    struct config_parse_state *s = data;
    unsigned int i;

    if (strcmp(name, "path") == 0) {
	if (!s->path_len)
	    ALOGW("Empty path\n");

	if (!s->dev) {
	    ALOGI("Applying %d element default route\n", s->path_len);

	    set_route_by_array(s->adev->mixer, s->path, s->path_len);

	    for (i = 0; i < s->path_len; i++) {
		free(s->path[i].ctl_name);
		free(s->path[i].strval);
	    }

	    free(s->path);

	    /* Refactor! */
	} else if (s->on) {
	    ALOGI("%d element on sequence\n", s->path_len);
	    s->dev->on = s->path;
	    s->dev->on_len = s->path_len;

	} else {
	    ALOGI("%d element off sequence\n", s->path_len);

	    /* Apply it, we'll reenable anything that's wanted later */
	    set_route_by_array(s->adev->mixer, s->path, s->path_len);

	    s->dev->off = s->path;
	    s->dev->off_len = s->path_len;
	}

	s->path_len = 0;
	s->path = NULL;

    } else if (strcmp(name, "device") == 0) {
	s->dev = NULL;
    }
}

static int adev_config_parse(struct tiny_audio_device *adev)
{
    struct config_parse_state s;
    FILE *f;
    XML_Parser p;
    char property[PROPERTY_VALUE_MAX];
    char file[80];
    int ret = 0;
    bool eof = false;
    int len;

    //property_get("ro.product.device", property, "tiny_hw");
    snprintf(file, sizeof(file), "/system/etc/%s", "tiny_hw.xml");

    ALOGV("Reading configuration from %s\n", file);
    f = fopen(file, "r");
    if (!f) {
        ALOGE("Failed to open %s\n", file);
        return -ENODEV;
    }

    p = XML_ParserCreate(NULL);
    if (!p) {
        ALOGE("Failed to create XML parser\n");
        ret = -ENOMEM;
        goto out;
    }

    memset(&s, 0, sizeof(s));
    s.adev = adev;
    XML_SetUserData(p, &s);

    XML_SetElementHandler(p, adev_config_start, adev_config_end);

    while (!eof) {
        len = fread(file, 1, sizeof(file), f);
        if (ferror(f)) {
            ALOGE("I/O error reading config\n");
            ret = -EIO;
            goto out_parser;
        }
        eof = feof(f);

        if (XML_Parse(p, file, len, eof) == XML_STATUS_ERROR) {
            ALOGE("Parse error at line %u:\n%s\n",
        	 (unsigned int)XML_GetCurrentLineNumber(p),
        	 XML_ErrorString(XML_GetErrorCode(p)));
            ret = -EINVAL;
            goto out_parser;
        }
    }

 out_parser:
    XML_ParserFree(p);
 out:
    fclose(f);

    return ret;
}

static void aud_vb_effect_start(struct tiny_audio_device *adev)
{
    if (adev)
        mixer_ctl_set_value(adev->private_ctl.vbc_eq_switch, 0, 1);
}

static void aud_vb_effect_stop(struct tiny_audio_device *adev)
{
    if (adev)
        mixer_ctl_set_value(adev->private_ctl.vbc_eq_switch, 0, 0);
}

/* Headset is 0, Handsfree is 3 */
static int get_mode_from_devices(int devices)
{
    int ret = 3;

    if ((devices & AUDIO_DEVICE_IN_BUILTIN_MIC) ||(devices & AUDIO_DEVICE_IN_BACK_MIC))
        ret = 3;
    else if (devices & AUDIO_DEVICE_IN_WIRED_HEADSET)
        ret = 0;

    return ret;
}
/*
 * Read audproc params from nv and config.
 * return value: TRUE:success, FALSE:failed
*/
static int init_rec_process(int rec_mode, int sample_rate)
{
    int audio_fd;
    int ret0 = 0; //failed
    int ret1 = 0;
    off_t offset = 0;
    AUDIO_TOTAL_T *aud_params_ptr = NULL;
    DP_CONTROL_PARAM_T *ctrl_param_ptr = 0;
    RECORDEQ_CONTROL_PARAM_T *eq_param_ptr = 0;
    unsigned int extendArraySize = 0;
    
    ALOGW("rec_mode(%d), sample_rate(%d)", rec_mode, sample_rate);
    audio_fd = open(ENG_AUDIO_PARA_DEBUG, O_RDONLY);
    if (-1 == audio_fd) {
        ALOGW("file %s open failed:%s\n",ENG_AUDIO_PARA_DEBUG,strerror(errno));
        audio_fd = open(ENG_AUDIO_PARA,O_RDONLY);
        if(-1 == audio_fd){
            ALOGE("file %s open error:%s\n",ENG_AUDIO_PARA,strerror(errno));
            return 0;
        }
    }else{
    //check the size of /data/local/tmp/audio_para
        offset = lseek(audio_fd,-1,SEEK_END);
        if((offset+1) != 4*sizeof(AUDIO_TOTAL_T)){
            ALOGE("%s, file %s size (%d) error \n",__func__,ENG_AUDIO_PARA_DEBUG,offset+1);
            close(audio_fd);
            audio_fd = open(ENG_AUDIO_PARA,O_RDONLY);
            if(-1 == audio_fd){
                ALOGE("%s, file %s open error:%s\n",__func__,ENG_AUDIO_PARA,strerror(errno));
                return 0;
            }
        }
    }
    aud_params_ptr = (AUDIO_TOTAL_T *)mmap(0, 4*sizeof(AUDIO_TOTAL_T),PROT_READ,MAP_SHARED,audio_fd,0);
    if ( NULL == aud_params_ptr ) {
        close(audio_fd);
        ALOGE("mmap failed %s",strerror(errno));
        return 0;
    }
    ctrl_param_ptr = (DP_CONTROL_PARAM_T *)((aud_params_ptr+rec_mode)->audio_enha_eq.externdArray);
 
    ret0 = AUDPROC_initDp(ctrl_param_ptr, sample_rate);

    //get total items of extend array.
    extendArraySize = sizeof((aud_params_ptr+rec_mode)->audio_enha_eq.externdArray);
    ALOGW("extendArraySize=%d, eq_size=%d, dp_size=%d",
                  extendArraySize, sizeof(RECORDEQ_CONTROL_PARAM_T), sizeof(DP_CONTROL_PARAM_T));
    if ((sizeof(RECORDEQ_CONTROL_PARAM_T) + sizeof(DP_CONTROL_PARAM_T)) <= extendArraySize)
    {
        eq_param_ptr =(RECORDEQ_CONTROL_PARAM_T *)&((aud_params_ptr+rec_mode)->audio_enha_eq.externdArray[19]);
        ret1 = AUDPROC_initRecordEq(eq_param_ptr, sample_rate);
    }else{
        ALOGE("Parameters error: No EQ params to init.");
    }
    
    munmap((void *)aud_params_ptr, 4*sizeof(AUDIO_TOTAL_T));
    close(audio_fd);
    
    return (ret0 || ret1);
}

static int aud_rec_do_process(void * buffer, size_t bytes)
{
    int16_t *temp_buf = NULL;
    size_t read_bytes = bytes;
    unsigned int dest_count = 0;

    temp_buf = (int16_t *) malloc(read_bytes);
    if (temp_buf) {
        AUDPROC_ProcessDp((int16 *) buffer, (int16 *) buffer, read_bytes >> 1, temp_buf, temp_buf, &dest_count);
        memcpy(buffer, temp_buf, read_bytes);
    } else {
        ALOGE("temp_buf malloc failed.(len=%d)", (int) read_bytes);
        return -1;
    }
    if (temp_buf)
        free(temp_buf);
    return 0;
}

static void *stream_routing_thread_entry(void * adev)
{
    struct tiny_audio_device *cur_adev = (struct tiny_audio_device *)adev;
    while(!cur_adev->routing_mgr.is_exit) {
        ALOGI("stream_routing_thread looping now...");
        sem_wait(&cur_adev->routing_mgr.device_switch_sem);
        do_select_devices(cur_adev);
        ALOGI("stream_routing_thread looping done.");
    }
    ALOGW("stream_routing_thread_entry exit!!!");
    return 0;
}

static int stream_routing_manager_create(struct tiny_audio_device *adev)
{
    int ret;

    adev->routing_mgr.is_exit = false;
    /* init semaphore to signal thread */
    ret = sem_init(&adev->routing_mgr.device_switch_sem, 0, 0);
    if (ret) {
        ALOGE("sem_init falied, code is %s", strerror(errno));
        return ret;
    }
    /* create a thread to manager the device routing switch.*/
    ret = pthread_create(&adev->routing_mgr.routing_switch_thread, NULL,
                            stream_routing_thread_entry, (void *)adev);
    if (ret) {
        ALOGE("pthread_create falied, code is %d", ret);
        return ret;
    }

    return ret;
}

static void stream_routing_manager_close(struct tiny_audio_device *adev)
{
    adev->routing_mgr.is_exit = true;
    /* release associated thread resource.*/
    sem_destroy(&adev->routing_mgr.device_switch_sem);
}

static int adev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    struct tiny_audio_device *adev;
    int ret;
    BLUE_TRACE("adev_open");
    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    adev = calloc(1, sizeof(struct tiny_audio_device));
    if (!adev)
        return -ENOMEM;
    memset(adev, 0, sizeof(struct tiny_audio_device));

    adev->hw_device.common.tag = HARDWARE_DEVICE_TAG;
    adev->hw_device.common.version = 0;
    adev->hw_device.common.module = (struct hw_module_t *) module;
    adev->hw_device.common.close = adev_close;

    adev->hw_device.get_supported_devices = adev_get_supported_devices;
    adev->hw_device.init_check = adev_init_check;
    adev->hw_device.set_voice_volume = adev_set_voice_volume;
    adev->hw_device.set_master_volume = adev_set_master_volume;
    adev->hw_device.set_mode = adev_set_mode;
    adev->hw_device.set_mic_mute = adev_set_mic_mute;
    adev->hw_device.get_mic_mute = adev_get_mic_mute;
    adev->hw_device.set_parameters = adev_set_parameters;
    adev->hw_device.get_parameters = adev_get_parameters;
    adev->hw_device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->hw_device.open_output_stream = adev_open_output_stream;
    adev->hw_device.close_output_stream = adev_close_output_stream;
    adev->hw_device.open_input_stream = adev_open_input_stream;
    adev->hw_device.close_input_stream = adev_close_input_stream;
    adev->hw_device.dump = adev_dump;

    /* query sound cards*/
    s_tinycard = get_snd_card_number(CARD_SPRDPHONE);
    s_vaudio = get_snd_card_number(CARD_VAUDIO);
    ALOGI("s_tinycard = %d, s_vaudio = %d", s_tinycard, s_vaudio);
    if (s_tinycard < 0 && s_vaudio < 0) {
        ALOGE("Unable to load sound card, aborting.");
        goto ERROR;
    }
    adev->mixer = mixer_open(s_tinycard);
    if (!adev->mixer) {
        ALOGE("Unable to open the mixer, aborting.");
        goto ERROR;
    }
    /* parse mixer ctl */
    ret = adev_config_parse(adev);
    if (ret < 0) {
        ALOGE("Unable to locate all mixer controls from XML, aborting.");
        goto ERROR;
    }
    BLUE_TRACE("ret=%d, num_dev_cfgs=%d", ret, adev->num_dev_cfgs);
    BLUE_TRACE("dev_cfgs_on depth=%d, dev_cfgs_off depth=%d", adev->dev_cfgs->on_len,  adev->dev_cfgs->off_len);

    /* generate eq params file of vbc effect*/
    adev->eq_available = false;
    ret = create_vb_effect_params();
    if (ret != 0) {
        ALOGW("Warning: Failed to create the parameters file of vbc_eq");
    } else {
        ret = mixer_ctl_set_enum_by_string(adev->private_ctl.vbc_eq_update, "loading");
        if (ret == 0) adev->eq_available = true;
        ALOGI("eq_loading, ret(%d), eq_available(%d)", ret, adev->eq_available);
    }
    if (adev->eq_available) {
        vb_effect_config_mixer_ctl(adev->private_ctl.vbc_eq_update, adev->private_ctl.vbc_eq_profile_select);
        aud_vb_effect_start(adev);
    }
    /*Parse PGA*/
    adev->pga = audio_pga_init(adev->mixer);
    if (!adev->pga) {
        ALOGE("Warning: Unable to locate PGA from XML.");
    }
    /* Set the default route before the PCM stream is opened */
    pthread_mutex_lock(&adev->lock);
    adev->mode = AUDIO_MODE_NORMAL;
    adev->devices = AUDIO_DEVICE_OUT_SPEAKER;
    select_devices_signal(adev);

    adev->pcm_modem_dl = NULL;
    adev->pcm_modem_ul = NULL;
    adev->call_start = 0;
    adev->call_connected = 0;
    adev->call_prestop = 0;
    adev->voice_volume = 1.0f;
    adev->bluetooth_nrec = false;

    pthread_mutex_unlock(&adev->lock);

    *device = &adev->hw_device.common;
#ifndef _VOICE_CALL_VIA_LINEIN
    /* Create a task to get vbpipe message from cp when voice-call */
    ret = vbc_ctrl_open(adev);
    if (ret < 0)  goto ERROR;
#endif
    ret = mmi_audio_loop_open();
    if (ret)  ALOGW("Warning: audio loop can NOT work.");
    
    ret = stream_routing_manager_create(adev);
    if (ret) {
        ALOGE("Unable to create stream_routing_manager, aborting.");
        goto ERROR;
    }
    return 0;

ERROR:
    if (adev->pga)    audio_pga_free(adev->pga);
    if (adev->mixer)  mixer_close(adev->mixer);
    if (adev)         free(adev);
    return -EINVAL;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "Spreadtrum Audio HW HAL",
        .author = "The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};

