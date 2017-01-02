#include "chip8.h"

chip8::chip8()
{
	this->Reset();
}

chip8::~chip8()
{

}

void chip8::Reset()
{
    run = false;
	pc			= 0x200; // program code starts at 0x200
	opcode		= 0;
	I			= 0;				
	sp			= 0;
	drawFlag	= false;
    frm = 0; //timer run every 9 cycles

	memset(this->gfx, 0, sizeof(this->gfx)); // black screen
	memset(this->stack, 0, sizeof(this->stack)); 
	memset(this->V, 0, sizeof(this->V));
	memset(this->memory, 0, sizeof(this->memory));
    memset(this->keypad, 0, sizeof(this->keypad));
    memcpy(memory, chip8_fontset, sizeof(chip8_fontset));

	delay_timer = 0;
	sound_timer = 0;
	srand(time(NULL));
}

void chip8::setLogger(void *log_func)
{
	logger = reinterpret_cast<log_cb>(reinterpret_cast<intptr_t>(log_func));
}

bool chip8::loadApplication(const void * data_, size_t size)
{
	const uint8_t *data = static_cast<const uint8_t*>(data_);
	if (data != NULL && size <= 0x1000 - 0x200)
	{
		for (size_t i = 0; i < size; ++i)
			this->memory[i + 0x200] = data[i];
		this->run = true;
		logger(CHIP8_LOG_INFO, "size %d", size);
		return true;
	}

	return false;
}

void chip8::runTimers()
{
	if (delay_timer > 0) // update timers
		--delay_timer;
	if (sound_timer > 0)
	{
		if (sound_timer == 1)
			logger(CHIP8_LOG_INFO, "PIIIIIIIIIIIIIIIIP\n");
		--sound_timer;
	}
}

void chip8::emulateCycle()
{
	if (!run)
		return;

	uint16_t  pee = pc;
	
	if (memory[pc] == 0x0 && memory[pc + 1] == 0x0 && memory[pc + 2] == 0x0 && memory[pc + 3] == 0x0)
	{
		logger(CHIP8_LOG_INFO, "Invalid pc 0x%X\n", pc); //shouldn't happen
		run = false;
	}
	
	++frm;

	opcode = memory[pc] << 8 | memory[pc + 1]; // fetch opcode
	//logger(CHIP8_LOG_INFO, "EMULOOP %d, op 0x%X\n",x++,opcode);		

	switch (opcode & 0xF000) // decode opcode
	{
	case 0x0000:
        if ((opcode & 0x00F0) >> 4 == 0xC)
        {
            logger(CHIP8_LOG_INFO, "Super chip8 unimplemented!");
            run = false;
        }
		switch (opcode & 0x00FF)
		{
		case 0x00E0: // 00E0: Clears the screen.
			memset(gfx, 0, sizeof(gfx));
			pc += 2;
			drawFlag = true;
			break;
		case 0x00EE: // 00EE: Returns from a subroutine.
            --sp;   //prevent overwrite
            if (sp < 0)
            {
                logger(CHIP8_LOG_INFO, "SP underrun!");
                run = false;
            }
			pc = stack[sp];
            pc += 2;
			break;
        case 0x00FB:
            logger(CHIP8_LOG_INFO, "Super chip8 unimplemented!");
            run = false;
            break;
        case 0x00FC:
            logger(CHIP8_LOG_INFO, "Super chip8 unimplemented!");
            run = false;
            break;
        case 0x00FD:
            logger(CHIP8_LOG_INFO, "Super chip8 unimplemented!");
            run = false;
            break;
        case 0x00FE:
            logger(CHIP8_LOG_INFO, "Super chip8 unimplemented!");
            run = false;
            break;
        case 0x00FF:
            logger(CHIP8_LOG_INFO, "Super chip8 unimplemented!");
            run = false;
            break;
		default:
			logger(CHIP8_LOG_INFO, "Unknown opcode [0x0000]: 0x%X\n", opcode);
			run = false;
		}
		break;
	case 0x1000: // 1NNN: Jumps to address NNN.
		pc = (opcode & 0x0FFF);
		break;
	case 0x2000: // 2NNN: Calls subroutine at NNN.
		stack[sp] = pc;
		++sp;
        if (sp > 15)
        {
            logger(CHIP8_LOG_INFO, "SP overrun!");
            run = false;
        }
        pc = (opcode & 0x0FFF);
		break;
	case 0x3000: // 3XNN: Skips the next instruction if VX equals NN. (Usually the next instruction is a jump to skip a code block)
		if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
			pc += 4;
        else
            pc += 2;
		break;
	case 0x4000: // 4XNN: Skips the next instruction if VX doesn't equal NN. (Usually the next instruction is a jump to skip a code block)
		if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
			pc += 4;
		else
            pc += 2;
		break;
	case 0x5000: // 5XY0: Skips the next instruction if VX equals VY. (Usually the next instruction is a jump to skip a code block)
		if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
			pc += 4;
        else
            pc += 2;
		break;
	case 0x6000: // 6XNN: Sets VX to NN.
		V[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
		pc += 2;
		break;
	case 0x7000: // 7XNN: Adds NN to VX.
		V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
		pc += 2;
		break;
	case 0x8000:
		switch (opcode & 0x000F)
		{
		case 0x0000: // 8XY0: Sets VX to the value of VY.
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 0x0001: // 8XY1: Sets VX to VX or VY. (Bitwise OR operation)
			V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 0x0002: // 8XY2: Sets VX to VX and VY. (Bitwise AND operation)
			V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 0x0003: // 8XY3: Sets VX to VX xor VY.
			V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 0x0004: // 8XY4: Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't.
			if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
				V[0xF] = 1; //carry
			else
				V[0xF] = 0;
			V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 0x0005: // 8XY5: VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
			if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
				V[0xF] = 0; // borrow
			else
				V[0xF] = 1;

			V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;
		case 0x0006: // 8XY6: Shifts VX right by one. VF is set to the value of the least significant bit of VX before the shift. On current implementations Y is ignored.
			V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
			V[(opcode & 0x0F00) >> 8] >>= 1;
			pc += 2;
			break;
		case 0x0007: // 8XY7: Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't.
			if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
				V[0xF] = 0;
			else
				V[0xF] = 1;

			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		case 0x000E: // 8XYE: Shifts VX left by one. VF is set to the value of the most significant bit of VX before the shift.
			V[0xF] = (V[(opcode & 0x0F00) >> 8] & 0x80) >> 7; // MSB, 0x80 in binary 0b10000000
			V[(opcode & 0x0F00) >> 8] <<= 1;
			pc += 2;
			break;
		default:
			logger(CHIP8_LOG_INFO, "Unknown opcode [0x8000]: 0x%X\n", opcode);
			run = false;
		}
		break;
	case 0x9000: // 9XY0: Skips the next instruction if VX doesn't equal VY. (Usually the next instruction is a jump to skip a code block)
		if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
			pc += 4;
		else
            pc += 2;
		break;
	case 0xA000: // ANNN: Sets I to the address NNN
		I = (opcode & 0x0FFF);
		pc += 2;
		break;
	case 0xB000: // BNNN: Jumps to the address NNN plus V0.
		pc = (opcode + V[0]) & 0x0FFF;
		break;
	case 0xC000: // CXNN: Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.
		V[(opcode & 0x0F00) >> 8] = ((rand()%0xFF) & (opcode & 0x00FF));
		pc += 2;
		break;
	case 0xD000:								 // DXYN: Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. Each row of 8 pixels is read as bit-coded starting from memory location I;
    {
        uint8_t vx = (opcode & 0x0F00) >> 8; // Need to bit shift by 8 to get to a single base16 digit.
        uint8_t vy = (opcode & 0x00F0) >> 4; // Need to bit shift by 4 to get to a single base16 digit.

        uint8_t xpixel = V[vx]; // Get x-pixel position from register Vx
        uint8_t ypixel = V[vy]; // Get y-pixel position from register Vy
        uint8_t nrows = (opcode & 0x000F); // Get num of rows to draw.
        uint32_t gfxarraypos = 0x0; // Variable used to calculate position within gfx memory array based on x and y positions.

        uint8_t newpixeldata = 0x0; // Variable used to hold the new pixel data from memory[I, I+1] etc.
        uint8_t oldpixelbit = 0x0; // Variable used to hold old pixel bit currently on screen.
        uint8_t newpixelbit = 0x0; // Variable used to hold new pixel bit, grabbed from the newpixeldata variable.

        logger(CHIP8_LOG_INFO, "DRAW X %d Y %d height %d\n", xpixel,ypixel,nrows);
        
        V[0xF] = 0; // Set VF to 0 initially (from specs).

        for (int ypos = 0; ypos < nrows; ypos++) { // Loop through number of rows to display from opcode.
            newpixeldata = memory[I + ypos]; // Get the first row of pixel data.
            for (int xpos = 0; xpos < 8; xpos++) { // Loop though the x-positions.
                gfxarraypos = ((ypixel + ypos) * 64) + (xpixel + xpos); // Calculate the gfx memory array position
                newpixelbit = (newpixeldata & (0x80 >> xpos)); // Get the pixel bit value from within the 8-bit data. (will be > 0 if there is a pixel)
                if (newpixelbit != 0) { // Set new pixel only if there is data.
                    oldpixelbit = gfx[gfxarraypos]; // Get the previous pixel value (already in the form of 1 or 0).
                    if (oldpixelbit == 1) V[0xF] = 1; // Set VF to 1 if pixel will be unset (from specs, used for collision detection).
                    gfx[gfxarraypos] = gfx[gfxarraypos] ^ 0x01; // Toggle pixel using XOR.
                }
            }
        }
        
		drawFlag = true;
		pc += 2;
		break;
	}
	case 0xE000:
		switch (opcode & 0x00FF)
		{
		case 0x00A1: // EXA1: Skips the next instruction if the key stored in VX isn't pressed. (Usually the next instruction is a jump to skip a code block)
			if (keypad[V[(opcode & 0x0F00) >> 8]] == 0)
				pc += 4;
			else
                pc += 2;
			break;
		case 0x009E: // EX9E: Skips the next instruction if the key stored in VX is pressed. (Usually the next instruction is a jump to skip a code block)
			if (keypad[V[(opcode & 0x0F00) >> 8]] != 0)
				pc += 4;
			else
                pc += 2;
			break;
		default:
			logger(CHIP8_LOG_INFO, "Unknown opcode [0x8000]: 0x%X\n", opcode);
			run = false;
		}
		break;
	case 0xF000:
		switch (opcode & 0x00FF)
		{
		case 0x0007: // FX07: Sets VX to the value of the delay timer.
			V[(opcode & 0x0F00) >> 8] = delay_timer;
			pc += 2;
			break;
		case 0x000A: // FX0A: A key press is awaited, and then stored in VX. (Blocking Operation. All instruction halted until next key event)
        {
            bool press = false;
            
            for (size_t i = 0; i < 16; ++i)
            {
                if (keypad[i] != 0)
                {
                    V[(opcode & 0x0F00) >> 8] = i;
                    press = true;
                }
            }
			if (!press)
                return;
            pc += 2;
			break;
        }
		case 0x0015: // FX15: Sets the delay timer to VX.
			delay_timer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		case 0x0018: // FX18: Sets the sound timer to VX.
			sound_timer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		case 0x001E: // FX1E: Adds VX to I. VF is set to 1 when range overflow
			if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF)
				V[0xF] = 1;
			else
				V[0xF] = 0;
			I += V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;
		case 0x0029: // FX29: Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font.
            logger(CHIP8_LOG_INFO, "SPRITE VX %X, index %d\n", V[(opcode & 0x0F00) >> 8], V[(opcode & 0x0F00) >> 8] * 0x5);
			I = V[(opcode & 0x0F00) >> 8] * 0x05;
			pc += 2;
			break;
		case 0x0033:												// FX33: Stores the binary-coded decimal representation of VX, with the most significant of three digits at the address in I, the middle digit at I plus 1, 
			memory[I] = V[(opcode & 0x0F00) >> 8] / 100;			//       and the least significant digit at I plus 2. (In other words, take the decimal representation of VX, place the hundreds digit in memory at location in I,
			memory[I + 1] = (V[(opcode & 0x0F00) >> 8] % 100) / 10;	//	      the tens digit at location I+1, and the ones digit at location I+2.)
			memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
            logger(CHIP8_LOG_INFO, "BCD V[%X] = %d, hundred %d ten %d one %d\n",(opcode & 0x0F00) >> 8,V[(opcode & 0x0F00) >> 8],memory[I],memory[I+1],memory[I+2]);
			pc += 2;
			break;
		case 0x0055: // FX55: Store registers V0 through Vx in memory starting at location I
			for (size_t i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
				memory[I + i] = V[i];
			I += ((opcode & 0x0F00) >> 8) + 1;
			pc += 2;
			break;
		case 0x0065: // FX65: Read registers V0 through Vx from memory starting at location I.
			for (size_t i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
				V[i] = memory[I + i];
			I += ((opcode & 0x0F00) >> 8) + 1;
			pc += 2;
			break;
		default:
			logger(CHIP8_LOG_INFO, "Unknown opcode [0xF000]: 0x%X\n", opcode);
			run = false;
		}
		break;
	default:
		logger(CHIP8_LOG_INFO, "Unknown opcode: 0x%X\n", opcode);
		run = false;
	}
	
	if (frm >= 9)
    {
        runTimers();
        frm = 0;
    }

	if (pee == pc)
	{
		logger(CHIP8_LOG_INFO, "Infinite loop detected at 0x%X, opcode 0x%X\n", pc, opcode);
		run = false;
	}
}

void * const chip8::getMemory()
{
	return memory;
}
