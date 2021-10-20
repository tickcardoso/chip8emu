#ifdef _WIN32
	#include <curses.h>
#else
	#include <ncurses.h>
#endif
#include "chip8.h"

void mem_debugger(size_t n) {
	size_t i;

	for(i=n; i<=MEM_SIZE; i++)
		printw("%02X ", memory[i]);

	addch('\n');
}

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

void gfx_debugger(void) {
	for(size_t i=0,j=0; i<SCREEN_WIDTH * SCREEN_HEIGHT; i++,j++) {
		if(j == SCREEN_WIDTH) {
			addch('\n');
			j = 0;
		}
		if(gfx[i] == 0)
			addch(' ');
		else
			printw("%x", gfx[i]);
	}
}

/* Initialize NCurses environment */
void init_debug(void) {
	initscr();
}

/* Terminate NCurses environment */
void free_debug(void) {
	endwin();
}
