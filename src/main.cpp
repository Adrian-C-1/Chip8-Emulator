#include <fstream>
#include <uchar.h>
#include <iomanip>
#include <sstream>
#include <bitset>
#include <chrono>
#include <random>
#include <thread>

#include <SDL2/SDL.h>

#include "screen.h"

#define TIMER_HZ 60

chip8::Screen *c8_screen = nullptr;

struct timer_base {
    std::chrono::high_resolution_clock::time_point counter;
    uint16_t reg;
    timer_base() {
        reg = 0;
    }
    uint16_t get() {
        return reg;
    }
    void set(uint16_t value) {
        reg = value;
        counter = std::chrono::high_resolution_clock::now();
    }
    void update() {
        if (reg == 0) return;
        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - counter).count();
        if (duration >= 1000.0 / TIMER_HZ) {
            counter = now;
            reg--;
        }
    }
};

unsigned char V[16];
uint16_t I;

unsigned char sprite[5 * 16] = {
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
uint16_t sprite_offset = 0x0;

unsigned char memory[4096];
uint16_t mem_offset = 0x200;

timer_base delay_timer;
timer_base audio_timer;

void logg(std::string message) {
    std::ofstream out("Files/log.txt", std::ios::out | std::ios::app);
    out << message << '\n';
    out.close();
}

void OC_DXYN(uint16_t instruction) {
    uint16_t X = (instruction & 0x0F00) >> 8;
    uint16_t Y = (instruction & 0x00F0) >> 4;
    for (int he = 0; he < (instruction & 0x000F); he++) {
        uint16_t screen_he = V[Y] + he; screen_he %= SCREEN_HEIGHT;
        uint16_t mem_loc = I + he;
        unsigned char c = memory[mem_loc];
        unsigned char mask = 0b10000000;
    
        for (int i = 0; i <= 7; i++) {
            bool bit = (c & mask) > 0;
            mask >>= 1;

            uint16_t screen_wi = V[X] + i; screen_wi %= SCREEN_WIDTH;
            if (screen[screen_he][screen_wi] == 1 && bit == 1) V[0xF] = 0x1;
            screen[screen_he][screen_wi] ^= bit;
            if (screen[screen_he][screen_wi] == 1) c8_screen->drawPixel(screen_wi, screen_he);
            else c8_screen->erasePixel(screen_wi, screen_he);
        }
    }
}

void preciseSleep(double seconds) { // not stolen code
	using namespace std;
	using namespace std::chrono;

	static double estimate = 5e-3;
	static double mean = 5e-3;
	static double m2 = 0;
	static int64_t count = 1;

	while (seconds > estimate) {
		auto start = high_resolution_clock::now();
		this_thread::sleep_for(milliseconds(1));
		auto end = high_resolution_clock::now();

		double observed = (end - start).count() / 1e9;
		seconds -= observed;

		++count;
		double delta = observed - mean;
		mean += delta / count;
		m2 += delta * (observed - mean);
		double stddev = sqrt(m2 / (count - 1));
		estimate = mean + stddev;
	}

	// spin lock
	auto start = high_resolution_clock::now();
	while ((high_resolution_clock::now() - start).count() / 1e9 < seconds);
}

// Who needs a stack when you got recursive loops
void loop(uint16_t location) { 
    std::chrono::high_resolution_clock::time_point frame_start;
    while (true) {
        std::chrono::high_resolution_clock::time_point frame_end = std::chrono::high_resolution_clock::now();
        float frame_ms = (std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start).count()) / 1000.0f;
        frame_start = frame_end;

        if (frame_ms < 1.0f / 60.0f){
            double should_delay_s = 1.0f / 60.0f - frame_ms; // in seconds
            preciseSleep(should_delay_s);
        }
        else{
            // welp
        }
        
        c8_screen->handleEvents();
        if (c8_screen->closed()) return;

        uint16_t instruction = memory[location++];
        instruction <<= 8;
        instruction += memory[location++];
        
        delay_timer.update();
        if (audio_timer.get() > 0)  _beep(500, 67); // Not using a sound library just for this
        audio_timer.update();

        switch (instruction & 0xF000)
        {
        case 0x0000:
            if (instruction == 0x00EE) { // 00EE -> Returns from a subroutine
                return;
            }
            else if (instruction == 0x00E0) c8_screen->clear(); // 00E0 -> Clears the screen
            else ; // 0NNN -> Calls machine code routine at address NNN (ignored by modern machines)
            break;
        case 0x1000:// 1NNN -> Jumps to address NNN
            location = instruction & 0x0FFF;
            break;
        case 0x2000:// 2NNN -> Calls subroutine at NNN
            loop(instruction & 0x0FFF);
            break;
        case 0x3000:// 3XNN -> Skips the next instruction if VX equals NN
            if (V[(instruction & 0x0F00) >> 8] == (instruction & 0x00FF))
                location += 2;
            break;
        case 0x4000:// 4XNN -> Skips the next instruction if VX does not equal NN
            if (V[(instruction & 0x0F00) >> 8] != (instruction & 0x00FF))
                location += 2;
            break;
        case 0x5000:// 5XY0 -> Skips the next instruction if VX equals VY
            if (V[(instruction & 0x0F00) >> 8] == V[(instruction & 0x00F0) >> 4])
                location += 2;
            break;
        case 0x6000:// 6XNN -> Sets VX to NN
            V[(instruction & 0x0F00) >> 8] = instruction & 0x00FF;
            break;
        case 0x7000:// 7XNN -> Adds NN to VX (carry flag is not changed)
            V[(instruction & 0x0F00) >> 8] += instruction & 0x00FF;
            break;
        case 0x8000:
            switch(instruction & 0x000F){
            case 0x0000:// 8XY0 -> Sets VX to the value of VY
                V[(instruction & 0x0F00) >> 8] = V[(instruction & 0x00F0) >> 4];
                break;
            case 0x0001:// 8XY1 -> Sets VX to VX or VY. (bitwise OR operation)
                V[(instruction & 0x0F00) >> 8] |= V[(instruction & 0x00F0) >> 4];
                break;
            case 0x0002:// 8XY2 -> Sets VX to VX and VY. (bitwise AND operation)
                V[(instruction & 0x0F00) >> 8] &= V[(instruction & 0x00F0) >> 4];
                break;
            case 0x0003:// 8XY3 -> Sets VX to VX xor VY
                V[(instruction & 0x0F00) >> 8] ^= V[(instruction & 0x00F0) >> 4];
                break;
            case 0x0004:// 8XY4 -> Adds VY to VX. VF is set to 1 when there's an overflow, and to 0 when there is not
                V[0xF] = (  (0xFF - V[(instruction & 0x0F00) >> 8]) < V[(instruction & 0x00F0) >> 4] ? 1 : 0  );
                V[(instruction & 0x0F00) >> 8] += V[(instruction & 0x00F0) >> 4];
                break;
            case 0x0005:// 8XY5 -> VY is subtracted from VX. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VX >= VY and 0 if not).
                V[0xF] = (  (V[(instruction & 0x00F0) >> 4] > V[(instruction & 0x0F00) >> 8]) ? 0 : 1  );
                V[(instruction & 0x0F00) >> 8] -= V[(instruction & 0x00F0) >> 4];
                break;
            case 0x0006:// 8XY6 -> Shifts VX to the right by 1, then stores the least significant bit of VX prior to the shift into VF
                V[0xF] = V[(instruction & 0x0F00) >> 8] & 0x1;
                V[(instruction & 0x0F00) >> 8] >>= 1;
                break;
            case 0x0007:// 8XY7 -> Sets VX to VY minus VX. VF is set to 0 when there's an underflow, and 1 when there is not. (i.e. VF set to 1 if VY >= VX)
                V[0xF] = (  (V[(instruction & 0x0F00) >> 8] > V[(instruction & 0x00F0) >> 4]) ? 0 : 1  );
                V[(instruction & 0x0F00) >> 8] = V[(instruction & 0x00F0) >> 4] - V[(instruction & 0x0F00) >> 8];
                break;
            case 0x000E:// 8XYE -> Shifts VX to the left by 1, then sets VF to 1 if the most significant bit of VX prior to that shift was set, or to 0 if it was unset.
                V[0xF] = ( (V[(instruction & 0x0F00) >> 8] >> 7) == 1 ? 1 : 0 );
                V[(instruction & 0x0F00) >> 8] <<= 1;
                break;
            }
            break;
        case 0x9000:// 9XY0 -> Skips the next instruction if VX does not equal VY
            if (V[(instruction & 0x0F00) >> 8] != V[(instruction & 0x00F0) >> 4])
                location += 2;
            break;
        case 0xA000:// ANNN -> Sets I to the address NNN
            I = instruction & 0x0FFF;
            break;
        case 0xB000:// BNNN -> Jumps to the address NNN plus V0
            location = (instruction & 0x0FFF) + V[0];
            break;
        case 0xC000:// CXNN -> Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN
            V[(instruction & 0x0F00) >> 8] = ((rand() % 0xFF) & (instruction & 0x00FF));
            break;
        case 0xD000:// DXYN -> Draw a sprite at Vx Vy of 8*N, start at I
            OC_DXYN(instruction);
            break;
        case 0xE000:
            switch(instruction & 0x00FF){
            case 0x009E:
                if (c8_screen->getKeyState(V[(instruction & 0x0F00) >> 8]) == 1)
                    location += 2;
                break;
            case 0x00A1:
                if (c8_screen->getKeyState(V[(instruction & 0x0F00) >> 8]) == 0)
                    location += 2;
                break;
            }
            break;
        case 0xF000:
        {
            switch(instruction & 0x00FF){
            case 0x0007:// FX07 -> Sets VX to the value of the delay timer
                V[(instruction & 0x0F00) >> 8] = delay_timer.get();
                break;
            case 0x000A:// FX0A -> A key press is awaited, and then stored in VX (blocking operation, all instruction halted until next key event)
                V[(instruction & 0x0F00) >> 8] = c8_screen->awaitKeyPress();
                break;
            case 0x0015:// FX15 -> Sets the delay timer to VX
                delay_timer.set(V[(instruction & 0x0F00) >> 8]);
                break;
            case 0x0018:// FX18 -> Sets the sound timer to VX
                audio_timer.set(V[(instruction & 0x0F00) >> 8]);
                break;
           case 0x001E:// FX1E -> Adds VX to I. VF is not affected
                I += V[(instruction & 0x0F00) >> 8];
                break;
            case 0x0029:// FX29 -> Sets I to the location of the sprite for the character in VX
                I = sprite_offset + 5 * V[(instruction & 0x0F00) >> 8];
                break;
            case 0x0033:// FX33 -> Stores the binary-coded decimal representation of VX in I 
                memory[I + 0] = V[(instruction & 0x0F00) >> 8] / 100;           // hundreds at I
                memory[I + 1] = (V[(instruction & 0x0F00) >> 8] % 100) / 10;    // tens at I + 1
                memory[I + 2] = V[(instruction & 0x0F00) >> 8] % 10;            // digits at I + 2
                break;
            case 0x055:// FX55 -> Stores from V0 to VX (including VX) in memory, starting at address I. The offset from I is increased by 1 for each value written, but I itself is left unmodified
                for (int k = 0; k <= ((instruction & 0x0F00) >> 8); k++) {
                    memory[I + k] = V[k];
                }
                break;
            case 0x0065:// FX65 -> Fills from V0 to VX (including VX) with values from memory, starting at address I. The offset from I is increased by 1 for each value read, but I itself is left unmodified
                for (int k = 0; k <= ((instruction & 0x0F00) >> 8); k++) {
                    V[k] = memory[I + k];
                }
                break;
            }
            break;
        }
        default:
            std::stringstream ss;
            ss << "Unimplemented: " << std::hex << std::setw(4) << std::setfill('0') << instruction << '\n';
            logg(ss.str());
            return;
        }
    
    }
}


int main(int argc, char* argv[]) {
    srand(time(NULL));
    SDL_Init(SDL_INIT_EVERYTHING);
    c8_screen = new chip8::Screen();

    std::ifstream fin("Files/config");
    std::string filename;
    std::getline(fin, filename);
    fin.close();

    fin.open("Files/" + filename, std::ios::in|std::ios::binary);
    if (!fin.is_open()) {
        logg("File does not open\n");
        return 0;
    }

    uint16_t nr = 0;
    unsigned char L, R;
    while (!fin.eof()) {
        fin.read(reinterpret_cast<char*>(&L), 1);
        fin.read(reinterpret_cast<char*>(&R), 1);

        memory[mem_offset + nr++] = L;
        memory[mem_offset + nr++] = R;
    }
    fin.close();

    for (int i = 0; i < 5 * 16; i++) {
        memory[sprite_offset + i] = sprite[i];
    }

    loop(0x200);

    return 0;
}