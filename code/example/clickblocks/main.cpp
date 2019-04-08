#include <SDL2/SDL.h>

void initSdl() {

    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window* window = SDL_CreateWindow("TextWindow", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024,768, SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_RenderClear(renderer);

    SDL_RenderPresent(renderer);

    SDL_Delay(500);

}

bool execute() {
    return false;
}

int main() {

    initSdl();

    while(execute());

}