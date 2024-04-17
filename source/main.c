#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <switch.h>

#define SAMPLERATE 48000
#define CHANNELCOUNT 2
#define FRAMERATE (1000 / 30)
#define SAMPLECOUNT (SAMPLERATE / FRAMERATE)
#define BYTESPERSAMPLE 2

void fill_audio_buffer(void * audio_buffer, size_t offset, size_t size, int frequency) {
  if (audio_buffer == NULL) return;

  u32 * dest = (u32 * ) audio_buffer;
  for (int i = 0; i < size; i++) {
    s16 sample = 0.3 * 0x7FFF * sin(frequency * (2 * M_PI) * (offset + i) / SAMPLERATE);
    dest[i] = (sample << 16) | (sample & 0xffff);
  }
}

int main(int argc, char ** argv) {  
  int volume, max, min;
  AudioTarget currentTarget;

  consoleInit(NULL);

  padConfigureInput(1, HidNpadStyleSet_NpadStandard);

  PadState pad;
  padInitializeDefault(&pad);

  AudioOutBuffer audout_buf;
  AudioOutBuffer * audout_released_buf;

  u32 data_size = (SAMPLECOUNT * CHANNELCOUNT * BYTESPERSAMPLE);
  u32 buffer_size = (data_size + 0xfff) & ~0xfff;

  u8 * out_buf_data = memalign(0x1000, buffer_size);

  audoutInitialize();
  audoutStartAudioOut();
  audctlInitialize();

  memset(out_buf_data, 0, buffer_size);

  fill_audio_buffer(out_buf_data, 0, data_size, 3500);

  bool play_tone = false;
  printf("Press X to force internal speaker output.\n");
  printf("Press R to increase volume.\n");
  printf("Press L to decrease volume.\n");
  printf("Press + to exit.\n");

  while (appletMainLoop()) {
    padUpdate(&pad);

    u64 kDown = padGetButtonsDown(&pad);
    if (kDown & HidNpadButton_Plus) break;

    if (kDown & HidNpadButton_X) {
      audctlSetDefaultTarget(AudioTarget_Speaker, 0, 0);
      play_tone = true;
      printf("Internal speaker output is now used. Redocking resets this setting.\n");
    }

    if (kDown & HidNpadButton_L) {
      audctlGetDefaultTarget(&currentTarget);
      if (currentTarget != AudioTarget_Speaker) {
        continue;
      }
      audctlGetTargetVolumeMin(&min);
      audctlGetTargetVolume(&volume, AudioTarget_Speaker);
      if (min == volume) {
        continue;
      }
      audctlSetTargetVolume(AudioTarget_Speaker, --volume);
      printf("Volume: %d\n", volume);
      play_tone = true;
    }

    if (kDown & HidNpadButton_R) {
      audctlGetDefaultTarget(&currentTarget);
      if (currentTarget != AudioTarget_Speaker) {
        continue;
      }
      audctlGetTargetVolumeMax(&max);
      audctlGetTargetVolume(&volume, AudioTarget_Speaker);
      if (max == volume) {
        continue;
      }
      audctlSetTargetVolume(AudioTarget_Speaker, ++volume);
      printf("Volume: %d\n", volume);
      play_tone = true;
    }

    if (play_tone) {
      audout_buf.next = NULL;
      audout_buf.buffer = out_buf_data;
      audout_buf.buffer_size = buffer_size;
      audout_buf.data_size = data_size;
      audout_buf.data_offset = 0;
      audout_released_buf = NULL;
      audoutPlayBuffer( & audout_buf, & audout_released_buf);
      play_tone = false;
    }

    consoleUpdate(NULL);
  }

  audoutStopAudioOut();


  audctlExit();
  audoutExit();
  consoleExit(NULL);
  return 0;
}