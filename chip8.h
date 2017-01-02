#include <cstdarg>
#include <memory>
#include <stdint.h>
#include <time.h>
//linux INT_MAX & memset & rand/srand
#include <climits>
#include <string.h>
#include <stdlib.h>

static const unsigned int SCREEN_X = 64;
static const unsigned int SCREEN_Y = 32;

static uint8_t chip8_fontset[80] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

enum CHIP8_LOG_LEVEL
{
	CHIP8_LOG_DEBUG = 0,
	CHIP8_LOG_INFO,
	CHIP8_LOG_WARN,
	CHIP8_LOG_ERROR,

	CHIP8_LOG_DUMMY = INT_MAX
};

class chip8
{
public:
	chip8();
	~chip8();

	void Reset();

	uint8_t gfx[SCREEN_X * SCREEN_Y];	// 64x32 pixel monochrome display
	uint8_t keypad[16];					// hex keypad

	bool drawFlag;
	bool run;

	void awaitKeypressComplete();
	void emulateCycle();
	bool loadApplication(const void * data, size_t size);
	void setLogger(void * log_func);

	void * const getMemory();
	void runTimers();

private:
	uint16_t pc;			// program counter
	uint16_t opcode;
	uint16_t I;				// index register
	uint16_t sp;			// stack pointer

	uint8_t V[16];			// V0-VF registers
	uint16_t stack[16];		// call stack
	uint8_t memory[0x1000];	// 4k memory

	uint8_t delay_timer;
	uint8_t sound_timer;
    
    uint8_t frm;            // for timer updates

	typedef void (*log_cb)(int level, const char *fmt, ...);
	log_cb logger;
};
