#ifndef _CHIP8_H_
#define _CHIP8_H_

	#include <stdint.h>
	#include <stdbool.h>

	/* 4096 bytes */
	#define MEM_SIZE 4096

	/* Program address range 0x200-0xFFF */
	#define PRG_ADDR 0x200

	/* The first 512 bytes is used by the interpreter itself.
	 * Only 3584 bytes of free memory to be used by program.
	 */
	#define FREE_MEM MEM_SIZE-PRG_ADDR
	#define DEBUG

	#define SCREEN_WIDTH 	64
	#define SCREEN_HEIGHT 32

	/* Structures */
	typedef struct {
		uint16_t I;
		uint16_t pc;
		uint16_t stack[16];
		uint16_t opcode;
		uint8_t V[16];
		uint8_t sp;
		uint8_t delay_timer;
		uint8_t sound_timer;
		bool draw_flag;
	} CHIP8;

	/* Global Variables */
	extern CHIP8 cpu;
	extern uint8_t memory[MEM_SIZE];
	extern uint8_t fontset[80];
	extern uint8_t gfx[SCREEN_WIDTH * SCREEN_HEIGHT];
	extern bool keys[16];

	/* Function Declarations */
	void init_chip8(void);
	void emulate_cycle(void);
	long fsize(FILE *fp);
	void copy_to_memory(FILE *fp);
	void load_rom(const char *n_game);

#endif
