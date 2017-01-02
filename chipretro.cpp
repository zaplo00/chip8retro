#include <ctime>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "libretro.h"
#include "chip8.h"

static uint16_t fb[2048];
static retro_usec_t frame_time;
static retro_usec_t time_reference;
static retro_usec_t total_time;
static struct retro_log_callback logging;
static retro_log_printf_t log_cb;
std::clock_t start;

static chip8 emu;

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
	(void)level;
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
}

void retro_init(void)
{
    memset(fb,0,sizeof(fb));
}

void retro_deinit(void)
{
	
}

unsigned retro_api_version(void)
{
	return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
	log_cb(RETRO_LOG_INFO, "Plugging device %u into port %u.\n", device, port);
}

void retro_get_system_info(struct retro_system_info *info)
{
	memset(info, 0, sizeof(*info));
	info->library_name = "CHIP8";
	info->library_version = "v1";
	info->need_fullpath = false;
	info->valid_extensions = NULL; // Anything is fine, we don't care.
}

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

void retro_get_system_av_info(struct retro_system_av_info *info)
{
	float aspect = 64 / 32;
	float sampling_rate = 44100;

	info->timing.fps = 60;
    info->timing.sample_rate = sampling_rate;

	info->geometry.base_width = SCREEN_X;
    info->geometry.base_height = SCREEN_Y;
    info->geometry.max_width = SCREEN_X;
    info->geometry.max_height = SCREEN_Y;
    info->geometry.aspect_ratio = aspect;
}

void retro_set_environment(retro_environment_t cb)
{
	environ_cb = cb;

	bool no_content = true;
	cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

	if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
		log_cb = logging.log;
	else
		log_cb = fallback_log;

	emu.setLogger((void*)log_cb);
}

static void frame_time_cb(retro_usec_t usec)
{
	frame_time = usec;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
	audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
	audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
	input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
	input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
	video_cb = cb;
}

void retro_reset(void)
{

}

static void update_input(void)
{
	input_poll_cb();
	emu.keypad[0x1] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_1) ? 1 : 0;
	emu.keypad[0x2] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_2) ? 1 : 0;
	emu.keypad[0x3] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_3) ? 1 : 0;
	emu.keypad[0xC] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_4) ? 1 : 0;

	emu.keypad[0x4] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_b) ? 1 : 0;
	emu.keypad[0x5] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_n) ? 1 : 0;
	emu.keypad[0x6] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_m) ? 1 : 0;
	emu.keypad[0xD] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_r) ? 1 : 0;

	emu.keypad[0x7] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_a) ? 1 : 0;
	emu.keypad[0x8] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_s) ? 1 : 0;
	emu.keypad[0x9] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_d) ? 1 : 0;
	emu.keypad[0xE] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_f) ? 1 : 0;

	emu.keypad[0xA] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_z) ? 1 : 0;
	emu.keypad[0x0] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_x) ? 1 : 0;
	emu.keypad[0xB] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_c) ? 1 : 0;
	emu.keypad[0xF] = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_v) ? 1 : 0;
}

static void check_variables(void)
{
}

static void audio_callback(void)
{
	audio_cb(1,1);
}

void retro_run(void)
{
		update_input();

		if (frame_time < (time_reference >> 1))
			total_time += frame_time;
		else
			total_time += ((frame_time + (time_reference >> 1)) / time_reference) * time_reference;
		int frames = (total_time + (time_reference >> 1)) / time_reference;

		if (frames <= 0)
		{
			video_cb(fb, SCREEN_X, SCREEN_Y, SCREEN_X << 1);
		}

		else if (total_time > time_reference)
		{			
			for (int i = 0; i < 10; ++i)
			{
				emu.emulateCycle();
				if (emu.drawFlag)
				{
                    for (int i=0; i < 2048; ++i)
                        fb[i] = emu.gfx[i] == 1 ?  0xFFFF : 0x0000;
					video_cb(fb, SCREEN_X, SCREEN_Y, SCREEN_X << 1); // 16bpp works
					emu.drawFlag ^= emu.drawFlag;
				}
			}

			total_time = 0;
		}

		audio_callback(); // nothing

		bool updated = false;
		if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
			check_variables();
		if (!emu.run)
			environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
}

bool retro_load_game(const struct retro_game_info *info)
{
	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
	if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
	{
		log_cb(RETRO_LOG_INFO, "RGB565 is not supported.\n");
		return false;
	}

	time_reference = 1000000 / 60; // some arbitrary value which works for whatever reason
	struct retro_frame_time_callback frame_cb = { frame_time_cb, time_reference };
	if (!environ_cb(RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK, &frame_cb))
	{
		log_cb(RETRO_LOG_INFO, "Failed to set frame time callback.\n");
		return false;
	}

	check_variables();

	if (info == NULL)
	{
		log_cb(RETRO_LOG_INFO, "*** Loaded CHIP8 core without game ***.\n");
		return true;
	}

	emu.loadApplication(info->data, info->size);

	return true;
}

void retro_unload_game(void)
{
	emu.Reset();
}

unsigned retro_get_region(void)
{
	return RETRO_REGION_PAL;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
	return false; //unused
}

size_t retro_serialize_size(void)
{
	return 0;
}

bool retro_serialize(void *data_, size_t size)
{	
	return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
	return false;
}

void *retro_get_memory_data(unsigned id)
{
	if (id == RETRO_MEMORY_SYSTEM_RAM)
		return emu.getMemory();
	return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
	if (id == RETRO_MEMORY_SYSTEM_RAM)
		return 0x1000;
	return 0;
}

void retro_cheat_reset(void)
{

}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
	(void)index;
	(void)enabled;
	(void)code;
}
