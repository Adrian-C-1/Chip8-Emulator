#include "screen.h"

bool screen[SCREEN_HEIGHT][SCREEN_WIDTH] = {};

int pixel_size = 10;
int current_screen_width = 800;
int current_screen_height = 500;

int screen_off_x = 0;
int screen_off_y = 0;

SDL_Window *window = nullptr;
SDL_Surface *surface = nullptr;
SDL_Event event;

std::map<unsigned char, bool> key_pressed = {};

void reload_screen()
{
    int max_pixel_x = current_screen_width / SCREEN_WIDTH;
    int max_pixel_y = current_screen_height / SCREEN_HEIGHT;
    pixel_size = std::min(max_pixel_x, max_pixel_y);

    SDL_Rect c8_screen;
    c8_screen.w = pixel_size * SCREEN_WIDTH;
    c8_screen.h = pixel_size * SCREEN_HEIGHT;
    c8_screen.x = current_screen_width / 2 - c8_screen.w / 2;
    c8_screen.y = current_screen_height / 2 - c8_screen.h / 2;

    screen_off_x = current_screen_width / 2 - c8_screen.w / 2;
    screen_off_y = current_screen_height / 2 - c8_screen.h / 2;

    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 50, 50, 50));
    for (int i = 0; i < SCREEN_HEIGHT; i++)
    {
        for (int j = 0; j < SCREEN_WIDTH; j++)
        {
            Uint32 color;
            if (screen[i][j] == 0)
                color = SDL_MapRGB(surface->format, 0, 0, 0);
            else
                color = SDL_MapRGB(surface->format, 255, 255, 255);
            SDL_Rect ree;
            ree.x = screen_off_x + pixel_size * j;
            ree.y = screen_off_y + pixel_size * i;
            ree.w = pixel_size - 1;
            ree.h = pixel_size - 1;
            SDL_FillRect(surface, &ree, color);
        }
    }
    SDL_UpdateWindowSurface(window);
}

int handleEventsInternal(void *userdata, SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_QUIT:
        SDL_DestroyWindow(window);
        window = nullptr;
        break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        unsigned char key = 0;
        switch (event->key.keysym.sym)
        {
        case SDLK_0:
            key = 0;
            break;
        case SDLK_1:
            key = 1;
            break;
        case SDLK_2:
            key = 2;
            break;
        case SDLK_3:
            key = 3;
            break;
        case SDLK_4:
            key = 4;
            break;
        case SDLK_5:
            key = 5;
            break;
        case SDLK_6:
            key = 6;
            break;
        case SDLK_7:
            key = 7;
            break;
        case SDLK_8:
            key = 8;
            break;
        case SDLK_9:
            key = 9;
            break;
        case SDLK_a:
            key = 10;
            break;
        case SDLK_b:
            key = 11;
            break;
        case SDLK_c:
            key = 12;
            break;
        case SDLK_d:
            key = 13;
            break;
        case SDLK_e:
            key = 14;
            break;
        case SDLK_f:
            key = 15;
            break;
        }
        key_pressed[key] = (event->type == SDL_KEYDOWN ? 1 : 0);
    }
    case SDL_WINDOWEVENT:
    {
        switch (event->window.event)
        {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        {
            surface = SDL_GetWindowSurface(window);
            int width = event->window.data1;
            int height = event->window.data2;
            current_screen_width = width;
            current_screen_height = height;

            reload_screen();

            break;
        }
        case SDL_WINDOWEVENT_RESIZED:
        {
            // God knows what triggers this event
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
    return 0;
}

namespace chip8
{

    Screen::Screen()
    {
        window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, current_screen_width, current_screen_height, SDL_WINDOW_RESIZABLE);
        surface = SDL_GetWindowSurface(window);

        SDL_AddEventWatch(handleEventsInternal, NULL);

        reload_screen();
    }

    bool Screen::getKeyState(unsigned int key)
    {
        if (key_pressed.find(key) == key_pressed.end())
            return 0;
        return key_pressed[key];
    }
    unsigned char Screen::awaitKeyPress()
    {
        while (true)
        {
            SDL_PollEvent(&event);
            if (event.type == SDL_KEYDOWN)
            {
                unsigned char key = 0;
                switch (event.key.keysym.sym)
                {
                case SDLK_0:
                    key = 0;
                    break;
                case SDLK_1:
                    key = 1;
                    break;
                case SDLK_2:
                    key = 2;
                    break;
                case SDLK_3:
                    key = 3;
                    break;
                case SDLK_4:
                    key = 4;
                    break;
                case SDLK_5:
                    key = 5;
                    break;
                case SDLK_6:
                    key = 6;
                    break;
                case SDLK_7:
                    key = 7;
                    break;
                case SDLK_8:
                    key = 8;
                    break;
                case SDLK_9:
                    key = 9;
                    break;
                case SDLK_a:
                    key = 10;
                    break;
                case SDLK_b:
                    key = 11;
                    break;
                case SDLK_c:
                    key = 12;
                    break;
                case SDLK_d:
                    key = 13;
                    break;
                case SDLK_e:
                    key = 14;
                    break;
                case SDLK_f:
                    key = 15;
                    break;
                default:
                    key = 100;
                    break;
                }

                if (key >= 0 && key <= 15)
                    return key;
            }
        }
    }
    void Screen::handleEvents()
    {
        while (SDL_PollEvent(&event))
            ;
    }
    bool Screen::closed()
    {
        return (window == nullptr);
    }

    void Screen::drawPixel(int x, int y)
    {
        screen[y][x] = 1;

        SDL_Rect rect;
        rect.x = screen_off_x + x * pixel_size;
        rect.y = screen_off_y + y * pixel_size;
        rect.w = pixel_size - 1;
        rect.h = pixel_size - 1;
        SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, 255, 255, 255));

        SDL_Rect rectt[1];
        rectt[0].x = screen_off_x + x * pixel_size;
        rectt[0].y = screen_off_y + y * pixel_size;
        rectt[0].w = pixel_size - 1;
        rectt[0].h = pixel_size - 1;
        SDL_UpdateWindowSurfaceRects(window, rectt, 1);
    }
    void Screen::erasePixel(int x, int y)
    {
        screen[y][x] = 0;

        SDL_Rect rect;
        rect.x = screen_off_x + x * pixel_size;
        rect.y = screen_off_y + y * pixel_size;
        rect.w = pixel_size - 1;
        rect.h = pixel_size - 1;
        SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, 0, 0, 0));

        SDL_Rect rectt[1];
        rectt[0].x = screen_off_x + x * pixel_size;
        rectt[0].y = screen_off_y + y * pixel_size;
        rectt[0].w = pixel_size - 1;
        rectt[0].h = pixel_size - 1;
        SDL_UpdateWindowSurfaceRects(window, rectt, 1);
    }
    void Screen::clear()
    {
        SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 0, 0));
    }

}
