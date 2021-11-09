#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

/* Soft reset CHIP-8 function */
void reset_chip8(void) {
  memset(gfx, 0, sizeof(uint8_t) * SCREEN_WIDTH * SCREEN_HEIGHT);
  cpu.pc = PRG_ADDR;
  cpu.draw_flag = true;
}

/* Initialize processor registers and memory */
void init_chip8(void) {
  memset(cpu.V, 0, sizeof(uint8_t) * 16);															/* Reset general-purpose registers to 0 */
  memset(cpu.stack, 0, sizeof(uint16_t) * 16);												/* Reset stack to 0 										*/
  memset(memory, 0, MEM_SIZE * sizeof(uint8_t));											/* Reset CHIP-8 memory to 0 						*/
  memset(keys, 0, sizeof(bool) * 16);																	/* Reset keys													  */
  memcpy(memory + 0x50, fontset, sizeof(fontset)/sizeof(*fontset));		/* Copy fontset to memory 						  */

  cpu.cycle_count = 0;
  cpu.I 	= cpu.opcode = cpu.sp = 0;
  cpu.delay_timer = cpu.sound_timer = 0;

  reset_chip8();
}


/* Get file size */
long fsize(FILE *fp) {
  long size;

  fseek(fp, 0L, SEEK_END);
  size = ftell(fp);

  rewind(fp);

  return size;
}

bool is_file(const char *filename) {
  const char *pos_ponto = strstr(filename, ".c8");

  if(pos_ponto == NULL)
    return false;
  if(pos_ponto == filename)
    return false;
  if(*(pos_ponto+1) == '\0')
    return false;

  return true;
}

void copy_to_memory(FILE *fp) {
  size_t sz = (size_t)fsize(fp);

  if(sz <= FREE_MEM)
    fread(memory + PRG_ADDR, sizeof(uint8_t), sz, fp);
  else {
    fprintf(stderr, "Game size exceeded free memory.\n");
    exit(1);
  }
}

void load_rom(const char *n_game) {
  FILE *fp;

  if(!is_file(n_game)) {
    fprintf(stderr, "File name doesn't match the pattern: filename.c8\n");
    exit(2);
  }

  if((fp = fopen(n_game, "rb")) == NULL) {
    fprintf(stderr, "File not found\n");
    exit(3);
  }

  copy_to_memory(fp);

  fclose(fp);
}

/* The interpreter reads N bytes from memory, starting at the address stored in I.
* These bytes are then displayed as sprites on screen at coordinates (VX, VY).
* Sprites are XORed onto the existing screen. If this causes any pixels to be erased,
* VF is set to 1, otherwise it is set to 0.
* If the sprite is positioned so part of it is outside the coordinates of the display,
* it wraps around to the opposite side of the screen.
*/
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

/* Opcode symbols:
*
* NNN    : address
* NN     : 8-bit constant
* N  		: 4-bit constant
* X and Y: 4-bit register identifier
* PC     : Program Counter
* I      : 16-bit register (for memory address)
* VN     : One of the 16 available variables. N may be 0 to F (hexadecimal)
*/

/* Emulate CPU cycle: fetch, decode, execute */
void emulate_cycle(void) {
  /* Fetch opcode */
  cpu.opcode = memory[cpu.pc] << 8 | memory[cpu.pc + 1];

  /* Decode and execute opcode */
  switch(cpu.opcode & 0xF000) {
    case 0x0000:
      switch(cpu.opcode & 0x00FF) {
        case 0x00E0:
          /* 00E0: Clears the screen */
          memset(gfx, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
          cpu.draw_flag = true;
          cpu.pc += 2;
          break;
        case 0x00EE:
          /* 00EE: Returns from a subroutine. */
          cpu.sp--;
          cpu.pc = cpu.stack[cpu.sp];
          cpu.pc += 2;
          break;
        default:
          fprintf(stderr, "Unknown opcode 0x%04X\n", cpu.opcode);
          exit(4);
      }
      break;
    case 0x1000:
      /* 1NNN: Jumps to address NNN. */
      cpu.pc = cpu.opcode & 0x0FFF;
      break;
    case 0x2000:
      /* 2NNN: Calls subroutine at NNN. */
      cpu.stack[cpu.sp] = cpu.pc;
      cpu.sp++;
      cpu.pc = cpu.opcode & 0x0FFF;
      break;
    case 0x3000:
      /* 3XNN: Skips the next instruction if VX equals to NN. */
      cpu.pc += (cpu.V[(cpu.opcode & 0x0F00) >> 8] == (cpu.opcode & 0x00FF)) ? 4 : 2;
      break;
    case 0x4000:
      /* 4XNN: Skips the next instruction if VX does not equal NN. */
      cpu.pc += (cpu.V[(cpu.opcode & 0x0F00) >> 8] != (cpu.opcode & 0x00FF)) ? 4 : 2;
      break;
    case 0x5000:
      /* 5XY0: Skips the next instruction if VX equals VY. */
      cpu.pc += (cpu.V[(cpu.opcode & 0x0F00) >> 8] == cpu.V[(cpu.opcode & 0x00F0) >> 4]) ? 4 : 2;
      break;
    case 0x6000:
      /* 6XNN: Sets VX to NN */
      cpu.V[(cpu.opcode & 0x0F00) >> 8] = cpu.opcode & 0x00FF;
      cpu.pc += 2;
      break;
    case 0x7000:
      /* 7XNN: Adds NN to VX. (Carry flag is not changed) */
      cpu.V[(cpu.opcode & 0x0F00) >> 8] += cpu.opcode & 0x00FF;
      cpu.pc += 2;
      break;
    case 0x8000:
      switch(cpu.opcode & 0x000F) {
        case 0x0000:
          /* 8XY0: Sets VX to the value of VY. */
          cpu.V[(cpu.opcode & 0x0F00) >> 8] = cpu.V[(cpu.opcode & 0x00F0) >> 4];
          cpu.pc += 2;
          break;
        case 0x0001:
          /* 8XY1: Sets VX to VX or VY. (Bitwise OR operation) */
          cpu.V[(cpu.opcode & 0x0F00) >> 8] |= cpu.V[(cpu.opcode & 0x00F0) >> 4];
          cpu.pc += 2;
          break;
        case 0x0002:
          /* 8XY2: Sets VX to VX and VY. (Bitwise AND operation) */
          cpu.V[(cpu.opcode & 0x0F00) >> 8] &= cpu.V[(cpu.opcode & 0x00F0) >> 4];
          cpu.pc += 2;
          break;
        case 0x0003:
          /* 8XY3: Sets VX to VX xor VY. */
          cpu.V[(cpu.opcode & 0x0F00) >> 8] ^= cpu.V[(cpu.opcode & 0x00F0) >> 4];
          cpu.pc += 2;
          break;
        case 0x0004:
          /* 8XY4: Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there is not. */
          if(cpu.V[(cpu.opcode & 0x00F0) >> 4] > (0xFF - cpu.V[(cpu.opcode & 0x0F00) >> 8]))
            cpu.V[0xF] = 1;
          else
            cpu.V[0xF] = 0;
          cpu.V[(cpu.opcode & 0x0F00) >> 8] += cpu.V[(cpu.opcode & 0x00F0) >> 4];
          cpu.pc += 2;
          break;
        case 0x0005:
          /* 8XY5: VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there is not. */
          if(cpu.V[(cpu.opcode & 0x0F00) >> 8] > cpu.V[(cpu.opcode & 0x00F0) >> 4])
            cpu.V[0xF] = 1;
          else
            cpu.V[0xF] = 0;
          cpu.V[(cpu.opcode & 0x0F00) >> 8] -= cpu.V[(cpu.opcode & 0x00F0) >> 4];
          cpu.pc += 2;
          break;
        case 0x0006:
          /* 8XY6: If the least-significant bit of VX is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2. */
          cpu.V[0xF] = cpu.V[(cpu.opcode & 0x0F00) >> 8] & 0x1;
          cpu.V[(cpu.opcode & 0x0F00) >> 8] >>= 1;
          cpu.pc += 2;
          break;
        case 0x0007:
          /* 8XY7: Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there is not. */
          if(cpu.V[(cpu.opcode & 0x0F00) >> 8] > cpu.V[(cpu.opcode & 0x00F0) >> 4])
            cpu.V[0xF] = 0;
          else
            cpu.V[0xF] = 1;
          cpu.V[(cpu.opcode & 0x0F00) >> 8] = cpu.V[(cpu.opcode & 0x00F0) >> 4] - cpu.V[(cpu.opcode & 0x0F00) >> 8];
          cpu.pc += 2;
          break;
        case 0x000E:
          /* 8XYE: If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2. */
          cpu.V[0xF] = cpu.V[(cpu.opcode & 0x0F00) >> 8] >> 7;
          cpu.V[(cpu.opcode & 0x0F00) >> 8] <<= 1;
          cpu.pc += 2;
          break;
        default:
          fprintf(stderr, "Unknown opcode 0x%04X\n", cpu.opcode);
          exit(4);
      }
      break;
    case 0x9000:
      /* 9XY0: Skips the next instruction if VX does not equal VY. */
      cpu.pc += (cpu.V[(cpu.opcode & 0x0F00) >> 8] != cpu.V[(cpu.opcode & 0x00F0) >> 4]) ? 4 : 2;
      break;
    case 0xA000:
      /* ANNN: Sets I (address register) to the address NNN */
      cpu.I = cpu.opcode & 0x0FFF;
      cpu.pc += 2;
      break;
    case 0xB000:
      /* BNNN: Jumps to the address NNN plus V0. */
      cpu.pc = cpu.V[0x0] + (cpu.opcode & 0x0FFF);
      break;
    case 0xC000:
      /* CXNN: Sets VX to the result of a bitwise and operation on a random number and NN.  */
      srand((unsigned int)time(NULL));
      cpu.V[(cpu.opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (cpu.opcode & 0x00FF);
      cpu.pc += 2;
      break;
    case 0xD000:
      /* DXYN: Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. */
      draw_sprite(cpu.V[(cpu.opcode & 0x0F00) >> 8], cpu.V[(cpu.opcode & 0x00F0) >> 4], cpu.opcode & 0x000F);
      cpu.draw_flag = true;
      cpu.pc += 2;
      break;
    case 0xE000:
      switch(cpu.opcode & 0x00FF) {
        case 0x009E:
          /* EX9E: Skips the next instruction if the key stored in VX is pressed. */
          cpu.pc += (keys[cpu.V[(cpu.opcode & 0x0F00) >> 8]] == true) ? 4 : 2;
          break;
        case 0x00A1:
          /* EXA1: Skips the next instruction if the key stored in VX is not pressed. */
          cpu.pc += (keys[cpu.V[(cpu.opcode & 0x0F00) >> 8]] == false) ? 4 : 2;
          break;
        default:
          fprintf(stderr, "Unknown opcode 0x%04X\n", cpu.opcode);
          exit(4);
      }
      break;
    case 0xF000:
      switch(cpu.opcode & 0x00FF) {
        case 0x0007:
          /* FX07: Sets VX to the value of the delay timer. */
          cpu.V[(cpu.opcode & 0x0F00) >> 8] = cpu.delay_timer;
          cpu.pc += 2;
          break;
        case 0x000A: {
          /* FX0A: A key press is awaited, and then stored in VX. (Blocking Operation.
          * All instruction halted until next key event).
          */
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
          /* FX15: Sets the delay timer to VX. */
          cpu.delay_timer = cpu.V[(cpu.opcode & 0x0F00) >> 8];
          cpu.pc += 2;
          break;
        case 0x0018:
          /* FX18: Sets the sound timer to VX. */
          cpu.sound_timer = cpu.V[(cpu.opcode & 0x0F00) >> 8];
          cpu.pc += 2;
          break;
        case 0x001E:
          /* FX1E: Adds VX to I.
          * Most CHIP-8 interpreters' FX1E instructions do not affect VF, with one exception:
          * The CHIP-8 interpreter for the Commodore Amiga sets VF to 1 when there is a range overflow (I+VX>0xFFF),
          * and to 0 when there is not.[15] The only known game that depends on this behavior is Spacefight 2091!
          * while at least one game, Animal Race, depends on VF not being affected.
          */
          if(cpu.I + cpu.V[(cpu.opcode & 0x0F00) >> 8] > 0xFFF)
            cpu.V[0xF] = 1;
          else
            cpu.V[0xF] = 0;
          cpu.I += cpu.V[(cpu.opcode & 0x0F00) >> 8];
          cpu.pc += 2;
          break;
        case 0x0029:
          /* FX29: Sets I to the location of the sprite for the character in VX.
          * Characters 0-F (in hexadecimal) are represented by a 4x5 font.
          */
          cpu.I = sprite_addr[cpu.V[(cpu.opcode & 0x0F00) >> 8]];
          cpu.pc += 2;
          break;
        case 0x0033:
          /* FX33: Store BCD representation of X in memory locations I, I+1, and I+2. */
          memory[cpu.I] 		= cpu.V[(cpu.opcode & 0x0F00) >> 8] / 100;
          memory[cpu.I + 1] = cpu.V[(cpu.opcode & 0x0F00) >> 8] % 100 / 10;
          memory[cpu.I + 2] = cpu.V[(cpu.opcode & 0x0F00) >> 8] % 100 % 10 / 1;
          cpu.pc += 2;
          break;
        case 0x0055:
        /* Stores V0 to VX (including VX) in memory starting at address I.
        * The offset from I is increased by 1 for each value written,
        * but I itself is left unmodified. */
          for(size_t i=0; i<=((cpu.opcode & 0x0F00) >> 8); i++)
            memory[cpu.I + i] = cpu.V[i];

          /* On the original interpreter, when the operation is done, I = I + X + 1. */
          cpu.I += cpu.V[(cpu.opcode & 0x0F00) >> 8] + 1;
          cpu.pc += 2;
          break;
        case 0x0065:
          /* Fills V0 to VX (including VX) with values from memory starting at address I.
          * The offset from I is increased by 1 for each value written,
          * but I itself is left unmodified. */
          for(size_t i=0; i<=((cpu.opcode & 0x0F00) >> 8); i++)
            cpu.V[i] = memory[cpu.I + i];

          /* On the original interpreter, when the operation is done, I = I + X + 1. */
          cpu.I += cpu.V[(cpu.opcode & 0x0F00) >> 8] + 1;
          cpu.pc += 2;
          break;
        default:
          fprintf(stderr, "Unknown opcode 0x04%X\n", cpu.opcode);
          exit(4);
      }
      break;
    default:
      fprintf(stderr, "Unknown opcode 0x%04X\n", cpu.opcode);
      exit(4);
  }

  /* Update timers */
  if(cpu.delay_timer > 0)
    cpu.delay_timer--;

  if(cpu.sound_timer > 0)
    cpu.sound_timer--;
}
