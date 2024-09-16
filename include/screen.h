#pragma once

#include <SDL2/SDL.h>
#include <map>

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

extern bool screen[SCREEN_HEIGHT][SCREEN_WIDTH];

extern int current_screen_width;
extern int current_screen_height;

extern SDL_Window *window;
extern SDL_Surface *surface;
extern SDL_Event event;

int handleEventsInternal(void *userdata, SDL_Event *event);

extern std::map<unsigned char, bool> key_pressed;

namespace chip8{

class Screen{
public:
    Screen();

    bool getKeyState(unsigned int key);
    unsigned char awaitKeyPress();
    void handleEvents();
    bool closed();


    void drawPixel(int x, int y);
    void erasePixel(int x, int y);
    void clear();
private:

};

}