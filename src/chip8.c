#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>
#include "chip8.h"

CHIP8 cpu;
uint8_t memory[MEM_SIZE];
uint8_t gfx[SCREEN_WIDTH * SCREEN_HEIGHT];
bool keys[16];

uint8_t fontset[] = {0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
										 0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
										 0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
										 0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
										 0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
										 0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
										 0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
										 0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
										 0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
										 0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
										 0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
										 0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
										 0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
										 0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
										 0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
										 0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */
};

uint8_t sprite_addr[] = {0x050, 0x055, 0x05A, 0x05F,
											   0x064, 0x069, 0x06E, 0x073,
												 0x078, 0x07D, 0x082, 0x087,
												 0x08C, 0x091, 0x096, 0x09B
};

/* Initialize processor registers and memory */
void initialize_chip8(void) {
	memset(cpu.V, 0, sizeof(uint8_t) * 16);															/* Reset general-purpose registers to 0 */
	memset(cpu.stack, 0, sizeof(uint16_t) * 16);												/* Reset stack to 0 										*/
	memset(memory, 0, MEM_SIZE * sizeof(uint8_t));											/* Reset CHIP-8 memory to 0 						*/
	memset(keys, 0, sizeof(bool) * 16);																	/* Reset keys													  */
	memset(gfx, 0, sizeof(uint8_t) * SCREEN_WIDTH * SCREEN_HEIGHT);			/* Reset internal display							  */
	memcpy(memory + 0x50, fontset, sizeof(fontset)/sizeof(*fontset));		/* Copy fontset to memory 						  */

	cpu.I 	= cpu.opcode = cpu.sp = 0;
	cpu.pc 	= PRG_ADDR;
	cpu.delay_timer = cpu.sound_timer = 0;
	cpu.draw_flag = true;
}

/* Get file size */
long fsize(FILE *fp) {
	long size;

	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);

	rewind(fp);

	return size;
}

void copy_to_memory(FILE *fp) {
	size_t sz = (size_t)fsize(fp);

	if(sz <= FREE_MEM)
		fread(memory + PRG_ADDR, sizeof(uint8_t), sz, fp);
	else {
		fprintf(stderr, "Game size exceeded free memory.\n");
		exit(2);
	}
}

void load_game(const char *n_game) {
	FILE *fp;

	if((fp = fopen(n_game, "rb")) == NULL) {
		fprintf(stderr, "File not found\n");
		exit(1);
	}

	copy_to_memory(fp);

	fclose(fp);
}

#ifdef CHIP8_DBG
// 	void mem_dbg(size_t n) {
// 		size_t i;
//
// 		for(i=n; i<=MEM_SIZE; i++)
// 			printf("%02X ", memory[i]);
//
// 		putchar('\n');
// 	}
//
/* Views processor registers */
void cpu_debugger(void) {
	uint16_t next_op = memory[cpu.pc] << 8 | memory[cpu.pc + 1];

	attron(A_BOLD);
	addstr("Registers\n");
	attroff(A_BOLD);

	for(size_t i=0; i<4; i++)
		printw("V%lX: %02X\t\tV%lX: %02X\t\tV%lX: %02X\t\tV%lX: %02X\n", i, cpu.V[i], i+0x4, cpu.V[i+0x4], i+0x8, cpu.V[i+0x8], i+0xC, cpu.V[i+0xC]);

	attron(A_BOLD);
	addstr("\nProcessor Status\n");
	attroff(A_BOLD);

	printw("PC: 0x%02X\tsp: 0x%X\n", cpu.pc, cpu.sp);
	printw("I : 0x%02X\n", cpu.I);
	printw("op: 0x%02X\tNext op.: 0x%02X\n", cpu.opcode, next_op);

	refresh();
	erase();
}
//
// 	void gfx_dbg(void) {
// 		for(size_t i=0,j=0; i<SCREEN_WIDTH * SCREEN_HEIGHT; i++,j++) {
// 			if(j == SCREEN_WIDTH) {
// 				putchar('\n');
// 				j = 0;
// 			}
// 			printf("%x", gfx[i]);
// 		}
// 		putchar('\n');
// 	}
#endif

void draw_sprite(uint8_t x, uint8_t y, uint8_t height) {
	uint8_t pixel;

	cpu.V[0xF] = 0;
	for(int yline=0; yline<height; yline++) {
		pixel = memory[cpu.I + yline];

		for(int xline=0; xline<8; xline++) {
			uint8_t posX = (x + xline) % SCREEN_WIDTH;
			uint8_t posY = (y + yline) % SCREEN_HEIGHT;
			uint16_t posPixel = (uint16_t)(posX + (posY * 64));

			if((pixel & (0x80 >> xline)) != 0) {
				if(gfx[posPixel] == 1)
					cpu.V[0xF] = 1;
				gfx[posPixel] ^= 1;
			}
		}
	}
}

/* Emulate CPU cycle: fetch, decode, execute */
void emulate_cycle(void) {
	/* Fetch opcode */
	cpu.opcode = memory[cpu.pc] << 8 | memory[cpu.pc + 1];

	/* Decode and execute opcode */
	switch(cpu.opcode & 0xF000) {
		case 0x0000:
			switch(cpu.opcode & 0x00FF) {
				case 0x00E0:
					memset(gfx, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
					cpu.draw_flag = true;
					cpu.pc += 2;
					break;
				case 0x00EE: /* Returns from a subroutine. */
					cpu.sp--;
					cpu.pc = cpu.stack[cpu.sp];
					cpu.pc += 2;
					break;
			}
			break;
		case 0x1000:
			cpu.pc = cpu.opcode & 0x0FFF;
			break;
		case 0x2000: /* Calls subroutine at NNN. */
			cpu.stack[cpu.sp] = cpu.pc;
			cpu.sp++;
			cpu.pc = cpu.opcode & 0x0FFF;
			break;
		case 0x3000: /* Skips the next instruction if VX equals to NN. */
			if(cpu.V[(cpu.opcode & 0x0F00) >> 8] == (cpu.opcode & 0x00FF))
				cpu.pc += 4;
			else
				cpu.pc += 2;
			break;
		case 0x4000: /* Skips the next instruction if VX does not equal NN. */
			if(cpu.V[(cpu.opcode & 0x0F00) >> 8] != (cpu.opcode & 0x00FF))
				cpu.pc += 4;
			else
				cpu.pc += 2;
			break;
		case 0x5000: /* Skips the next instruction if VX equals VY. */
			if(cpu.V[(cpu.opcode & 0x0F00) >> 8] == cpu.V[(cpu.opcode & 0x00F0) >> 4])
				cpu.pc += 4;
			else
				cpu.pc += 2;
			break;
		case 0x6000: /* Sets VX to NN */
			cpu.V[(cpu.opcode & 0x0F00) >> 8] = cpu.opcode & 0x00FF;
			cpu.pc += 2;
			break;
		case 0x7000: /* Adds NN to VX. (Carry flag is not changed) */
			cpu.V[(cpu.opcode & 0x0F00) >> 8] += cpu.opcode & 0x00FF;
			cpu.pc += 2;
			break;
		case 0x8000:
			switch(cpu.opcode & 0x000F) {
				case 0x0000: /* Sets VX to the value of VY. */
					cpu.V[(cpu.opcode & 0x0F00) >> 8] = cpu.V[(cpu.opcode & 0x00F0) >> 4];
					cpu.pc += 2;
					break;
				case 0x0001: /* Sets VX to VX or VY. (Bitwise OR operation) */
					cpu.V[(cpu.opcode & 0x0F00) >> 8] |= cpu.V[(cpu.opcode & 0x00F0) >> 4];
					cpu.pc += 2;
					break;
				case 0x0002: /* Sets VX to VX and VY. (Bitwise AND operation) */
					cpu.V[(cpu.opcode & 0x0F00) >> 8] &= cpu.V[(cpu.opcode & 0x00F0) >> 4];
					cpu.pc += 2;
					break;
				case 0x0003: /* Sets VX to VX xor VY. */
					cpu.V[(cpu.opcode & 0x0F00) >> 8] ^= cpu.V[(cpu.opcode & 0x00F0) >> 4];
					cpu.pc += 2;
					break;
				case 0x0004: /* Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there is not. */
					if(cpu.V[(cpu.opcode & 0x00F0) >> 4] > (0xFF - cpu.V[(cpu.opcode & 0x0F00) >> 8]))
						cpu.V[0xF] = 1;
					else
						cpu.V[0xF] = 0;
					cpu.V[(cpu.opcode & 0x0F00) >> 8] += cpu.V[(cpu.opcode & 0x00F0) >> 4];
					cpu.pc += 2;
					break;
				case 0x0005: /* VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there is not. */
					if(cpu.V[(cpu.opcode & 0x0F00) >> 8] > cpu.V[(cpu.opcode & 0x00F0) >> 4])
						cpu.V[0xF] = 1;
					else
						cpu.V[0xF] = 0;
					cpu.V[(cpu.opcode & 0x0F00) >> 8] -= cpu.V[(cpu.opcode & 0x00F0) >> 4];
					cpu.pc += 2;
					break;
				case 0x0006: /* If the least-significant bit of VX is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2. */
					cpu.V[0xF] = cpu.V[(cpu.opcode & 0x0F00) >> 8] & 0x1;
					cpu.V[(cpu.opcode & 0x0F00) >> 8] >>= 1;
					cpu.pc += 2;
					break;
				case 0x0007: /* Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there is not. */
					if(cpu.V[(cpu.opcode & 0x0F00) >> 8] > cpu.V[(cpu.opcode & 0x00F0) >> 4])
						cpu.V[0xF] = 0;
					else
						cpu.V[0xF] = 1;
					cpu.V[(cpu.opcode & 0x0F00) >> 8] = cpu.V[(cpu.opcode & 0x00F0) >> 4] - cpu.V[(cpu.opcode & 0x0F00) >> 8];
					cpu.pc += 2;
					break;
				case 0x000E: /* If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2. */
					cpu.V[0xF] = cpu.V[(cpu.opcode & 0x0F00) >> 8] >> 7;
					cpu.V[(cpu.opcode & 0x0F00) >> 8] <<= 1;
					cpu.pc += 2;
					break;
			}
			break;
		case 0x9000: /* Skips the next instruction if VX does not equal VY. */
			if(cpu.V[(cpu.opcode & 0x0F00) >> 8] != cpu.V[(cpu.opcode & 0x00F0) >> 4])
				cpu.pc += 4;
			else
				cpu.pc += 2;
			break;
		case 0xA000: /* Sets I (address register) to the address NNN */
			cpu.I = cpu.opcode & 0x0FFF;
			cpu.pc += 2;
			break;
		case 0xB000: /* Jumps to the address NNN plus V0. */
			cpu.pc = cpu.V[0x0] + (cpu.opcode & 0x0FFF);
			break;
		case 0xC000: /* Sets VX to the result of a bitwise and operation on a random number and NN.  */
			srand((unsigned int)time(NULL));
			cpu.V[(cpu.opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (cpu.opcode & 0x00FF);
			cpu.pc += 2;
			break;
		case 0xD000:
			draw_sprite(cpu.V[(cpu.opcode & 0x0F00) >> 8], cpu.V[(cpu.opcode & 0x00F0) >> 4], cpu.opcode & 0x000F);
			cpu.draw_flag = true;
			cpu.pc += 2;
			break;
		case 0xE000:
			switch(cpu.opcode & 0x00FF) {
				case 0x009E:
					if(keys[cpu.V[(cpu.opcode & 0x0F00) >> 8]] == true)
						cpu.pc += 4;
					else
						cpu.pc += 2;
					break;
				case 0x00A1:
					if(keys[cpu.V[(cpu.opcode & 0x0F00) >> 8]] == false)
						cpu.pc += 4;
					else
						cpu.pc += 2;
					break;
			}
			break;
		case 0xF000:
			switch(cpu.opcode & 0x00FF) {
				case 0x0007:
					cpu.V[(cpu.opcode & 0x0F00) >> 8] = cpu.delay_timer;
					cpu.pc += 2;
					break;
				case 0x000A: {
						bool key_press = false;

						for(uint8_t i=0; i<16; i++) {
							if(keys[i] != false) {
								cpu.V[(cpu.opcode & 0x0F00) >> 8] = i;
								key_press = true;
							}
						}

						if(!key_press)
							return;

						cpu.pc += 2;
					}
					break;
				case 0x0015:
					cpu.delay_timer = cpu.V[(cpu.opcode & 0x0F00) >> 8];
					cpu.pc += 2;
					break;
				case 0x0018:
					cpu.sound_timer = cpu.V[(cpu.opcode & 0x0F00) >> 8];
					cpu.pc += 2;
					break;
				case 0x001E:
					if(cpu.I + cpu.V[(cpu.opcode & 0x0F00) >> 8] > 0xFFF)
						cpu.V[0xF] = 1;
					else
						cpu.V[0xF] = 0;
					cpu.I += cpu.V[(cpu.opcode & 0x0F00) >> 8];
					cpu.pc += 2;
					break;
				case 0x0029:
					cpu.I = sprite_addr[cpu.V[(cpu.opcode & 0x0F00) >> 8]];
					cpu.pc += 2;
					break;
				case 0x0033:
					memory[cpu.I] 		= cpu.V[(cpu.opcode & 0x0F00) >> 8] / 100;
					memory[cpu.I + 1] = cpu.V[(cpu.opcode & 0x0F00) >> 8] % 100 / 10;
					memory[cpu.I + 2] = cpu.V[(cpu.opcode & 0x0F00) >> 8] % 100 % 10 / 1;
					cpu.pc += 2;
					break;
				case 0x0055:
				/* Stores V0 to VX (including VX) in memory starting at address I.
				 * The offset from I is increased by 1 for each value written,
				 * but I itself is left unmodified. */
					for(uint16_t i=0; i<=((cpu.opcode & 0x0F00) >> 8); i++)
						memory[cpu.I + i] = cpu.V[i];

					/* On the original interpreter, when the operation is done, I = I + X + 1. */
					cpu.I += cpu.V[(cpu.opcode & 0x0F00) >> 8] + 1;
					cpu.pc += 2;
					break;
				case 0x0065:
					/* Fills V0 to VX (including VX) with values from memory starting at address I.
					 * The offset from I is increased by 1 for each value written,
					 * but I itself is left unmodified. */
					for(uint16_t i=0; i<=((cpu.opcode & 0x0F00) >> 8); i++)
						cpu.V[i] = memory[cpu.I + i];

					/* On the original interpreter, when the operation is done, I = I + X + 1. */
					cpu.I += cpu.V[(cpu.opcode & 0x0F00) >> 8] + 1;
					cpu.pc += 2;
					break;
			}
			break;
		default:
			fprintf(stderr, "Unknown opcode 0x%X\n", cpu.opcode);
			exit(3);
	}

	/* Update timers */
	if(cpu.delay_timer > 0)
		cpu.delay_timer--;

	if(cpu.sound_timer > 0)
		cpu.sound_timer--;
}
