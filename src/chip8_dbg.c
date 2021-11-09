#ifdef _WIN32
  #include <curses.h>
#else
  #include <ncurses.h>
#endif
#include "chip8.h"

void disassembler(uint16_t opcode) {
  switch(opcode & 0xF000) {
    case 0x0000:
      printw("SYS %03X", opcode & 0x0FFF);
      break;
    case 0x00E0:
      printw("CLS");
      break;
    case 0x00EE:
      printw("RET");
      break;
    case 0x1000:
      printw("JP %03X", opcode & 0x0FFF);
      break;
    case 0x2000:
      printw("CALL %03X", opcode & 0x0FFF);
      break;
    case 0x3000:
      printw("SE V%X, %02X", (opcode & 0x0F00) >> 8, opcode & 0x00FF);
      break;
    case 0x4000:
      printw("SNE V%X, %02X", (opcode & 0x0F00) >> 8, opcode & 0x00FF);
      break;
    case 0x5000:
      printw("SE V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
      break;
    case 0x6000:
      printw("LD V%X, %02X", (opcode & 0x0F00) >> 8, opcode & 0x00FF);
      break;
    case 0x7000:
      printw("ADD V%X, %02X", (opcode & 0x0F00) >> 8, opcode & 0x00FF);
      break;
    case 0x8000:
      switch(opcode & 0x000F) {
        case 0x0000:
          printw("LD V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
          break;
        case 0x0001:
          printw("OR V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
          break;
        case 0x0002:
          printw("AND V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
          break;
        case 0x0003:
          printw("XOR V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
          break;
        case 0x0004:
          printw("ADD V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
          break;
        case 0x0005:
          printw("SUB V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
          break;
        case 0x0006:
          printw("SHR V%X {, V%X}", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
          break;
        case 0x0007:
          printw("SUBN V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
          break;
        case 0x000E:
          printw("SHL V%X, {, V%X}", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
          break;
        default:
          printw("Unknown opcode 0x%04X", opcode);
      }
      break;
    case 0x9000:
      printw("SNE V%X, V%X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4);
      break;
    case 0xA000:
      printw("LD I, %03X", opcode & 0x0FFF);
      break;
    case 0xB000:
      printw("JP V0, %03X", opcode & 0x0FFF);
      break;
    case 0xC000:
      printw("RND V%X, %02X", (opcode & 0x0F00) >> 8, opcode & 0x0FF);
      break;
    case 0xD000:
      printw("DRW V%X, V%X, %X", (opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4, opcode & 0x000F);
      break;
    case 0xE000:
      switch(opcode & 0x00FF) {
        case 0x009E:
          printw("SKP V%X", (opcode & 0x0F00) >> 8);
          break;
        case 0x00A1:
          printw("SKNP V%X", (opcode & 0x0F00) >> 8);
          break;
        default:
          printw("Unknown opcode 0x%04X", opcode);
      }
      break;
    case 0xF000:
      switch(opcode & 0x00FF) {
        case 0x0007:
          printw("LD V%X, DT", (opcode & 0x0F00) >> 8);
          break;
        case 0x0015:
          printw("LD DT, V%X", (opcode & 0x0F00) >> 8);
          break;
        case 0x0018:
          printw("LD ST V%X", (opcode & 0x0F00) >> 8);
          break;
        case 0x001E:
          printw("ADD I, V%X", (opcode & 0x0F00) >> 8);
          break;
        case 0x0029:
          printw("LD F, V%X", (opcode & 0x0F00) >> 8);
          break;
        case 0x0033:
          printw("LD B, V%X", (opcode & 0x0F00) >> 8);
          break;
        case 0x0055:
          printw("LD [I], V%X", (opcode & 0x0F00) >> 8);
          break;
        case 0x0065:
          printw("LD V%X, [I]", (opcode & 0x0F00) >> 8);
          break;
        default:
          printw("Unknown opcode 0x%04X", opcode);
      }
      break;
    default:
      printw("Unknown opcode 0x%04X", opcode);
  }
}

void mem_debugger(size_t n) {
  size_t i;

  for(i=n; i<=MEM_SIZE; i++)
    printw("%02X ", memory[i]);

  addch('\n');
}

/* Views processor registers */
void cpu_debugger(void) {
  attron(A_BOLD);
  addstr("Registers\n");
  attroff(A_BOLD);

  for(size_t i=0; i<4; i++)
    printw("V%lX: %02X\t\tV%lX: %02X\t\tV%lX: %02X\t\tV%lX: %02X\n", i, cpu.V[i], i+0x4, cpu.V[i+0x4], i+0x8, cpu.V[i+0x8], i+0xC, cpu.V[i+0xC]);

  attron(A_BOLD);
  addstr("\nProcessor Status\n");
  attroff(A_BOLD);

  printw("PC: 0x%02X\tsp: 0x%X\t\tI: 0x%02X\n", cpu.pc, cpu.sp, cpu.I);
  printw("Cycles: %u\n", cpu.cycle_count);

  printw("op: ");
  disassembler(cpu.opcode);
  printw(" (0x%02X)", cpu.opcode);

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
