#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "chip8.h"

#ifdef DEBUG
  #include "chip8_dbg.h"
#endif

#define L_WIDTH 1024
#define L_HEIGHT 512

#define M_PI        3.14159265358979323846
#define AMPLITUDE   28000
#define SAMPLE_RATE 44100

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
SDL_Event event;

void setup_graphics(void);
void key_down(SDL_Event *event);
void key_up(SDL_Event *event);
void setup_audio(void);
void update_screen(void);
void destroy_emu(void);

int main(int argc, char *argv[]) {
  bool quit = false;

  if(argc != 2) {
    printf("Usage: %s rom_file\n", argv[0]);
    exit(10);
  }

  init_chip8();

  load_rom(argv[argc-1]);

  init_debug();
  setup_graphics();
  setup_audio();

  // Main loop
  while(!quit) {
    emulate_cycle();
    cpu_debugger();

    while(SDL_PollEvent(&event)) {
      if(event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
        quit = true;
      else
        if(event.type == SDL_KEYDOWN)
          key_down(&event);
        else
          if(event.type == SDL_KEYUP)
            key_up(&event);
    }

    if(cpu.draw_flag) {
      cpu.draw_flag = false;

      update_screen();
    }

    SDL_PauseAudio(!cpu.sound_timer);

    cpu.cycle_count++;
  }

  destroy_emu();
  free_debug();

  return 0;
}

void setup_graphics(void) {
  if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    exit(11);
  }

  window = SDL_CreateWindow("CHIP-8 Emulator",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            L_WIDTH, L_HEIGHT,
                            SDL_WINDOW_SHOWN);
  if(window == NULL) {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    exit(12);
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  SDL_RenderSetLogicalSize(renderer, L_WIDTH, L_HEIGHT);

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);
}

void key_down(SDL_Event *event) {
  switch(event->key.keysym.sym) {
    case SDLK_x:
      keys[0x0] = true;
      break;
    case SDLK_1:
      keys[0x1] = true;
      break;
    case SDLK_2:
      keys[0x2] = true;
      break;
    case SDLK_3:
      keys[0x3] = true;
      break;
    case SDLK_q:
      keys[0x4] = true;
      break;
    case SDLK_w:
      keys[0x5] = true;
      break;
    case SDLK_e:
      keys[0x6] = true;
      break;
    case SDLK_a:
      keys[0x7] = true;
      break;
    case SDLK_s:
      keys[0x8] = true;
      break;
    case SDLK_d:
      keys[0x9] = true;
      break;
    case SDLK_z:
      keys[0xA] = true;
      break;
    case SDLK_c:
      keys[0xB] = true;
      break;
    case SDLK_4:
      keys[0xC] = true;
      break;
    case SDLK_r:
      keys[0xD] = true;
      break;
    case SDLK_f:
      keys[0xE] = true;
      break;
    case SDLK_v:
      keys[0xF] = true;
      break;
    case SDLK_u:
      reset_chip8();
  }
}

void key_up(SDL_Event *event) {
  switch(event->key.keysym.sym) {
    case SDLK_x:
      keys[0x0] = false;
      break;
    case SDLK_1:
      keys[0x1] = false;
      break;
    case SDLK_2:
      keys[0x2] = false;
      break;
    case SDLK_3:
      keys[0x3] = false;
      break;
    case SDLK_q:
      keys[0x4] = false;
      break;
    case SDLK_w:
      keys[0x5] = false;
      break;
    case SDLK_e:
      keys[0x6] = false;
      break;
    case SDLK_a:
      keys[0x7] = false;
      break;
    case SDLK_s:
      keys[0x8] = false;
      break;
    case SDLK_d:
      keys[0x9] = false;
      break;
    case SDLK_z:
      keys[0xA] = false;
      break;
    case SDLK_c:
      keys[0xB] = false;
      break;
    case SDLK_4:
      keys[0xC] = false;
      break;
    case SDLK_r:
      keys[0xD] = false;
      break;
    case SDLK_f:
      keys[0xE] = false;
      break;
    case SDLK_v:
      keys[0xF] = false;
      break;
  }
}

/* SDL Audio Callback */
void audio_callback(void *user_data, uint8_t *raw_buffer, int bytes) {
  int16_t *buffer = (int16_t *)raw_buffer;
  int length = bytes / 2;  /* 2 bytes per sample for AUDIO_S16SYS */
  int sample_nr = (*(int *)user_data);

  for(int i=0; i<length; i++,sample_nr++) {
    double time = (double)sample_nr / (double)SAMPLE_RATE;

    /* Render 441 HZ sine wave */
    buffer[i] = (int16_t)(AMPLITUDE * sin(2.0f * M_PI * 441.0f * time));
  }
}

void setup_audio(void) {
  int sample_nr = 0;
  SDL_AudioSpec want;
  SDL_AudioSpec have;

  /* Number of sample per second */
  want.freq = SAMPLE_RATE;

  /* Sample type (here: signed short i.e. 16 bit) */
  want.format = AUDIO_S16SYS;

  /* Only one channel */
  want.channels = 1;

  /* Buffer size */
  want.samples = 2048;

  /* Function SDL calls periodically to refill the buffer */
  want.callback = audio_callback;

  /* Counter, keeping track of current sample number */
  want.userdata = &sample_nr;

  if(SDL_OpenAudio(&want, &have) != 0)
    printf("Could not open audio: %s.\n", SDL_GetError());

  if(want.format != have.format)
    printf("Could not get desired audio spec.\n");
}

void update_screen(void) {
  uint32_t pixels[2048];

  for(int i=0; i<2048; i++) {
    uint8_t pixel = gfx[i];
    pixels[i] = (0x00FFFFFF * pixel) | 0xFF000000;
  }

  SDL_UpdateTexture(texture, NULL, pixels, 64 * sizeof(uint32_t));

  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void destroy_emu(void) {
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_CloseAudio();
  SDL_Quit();
}
