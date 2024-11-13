#include <SDL3/SDL.h>
#include <SDL3_shadercross/SDL_shadercross.h>

int main(int argc, char *argv[]) {
    if (!SDL_Init(0)) {
        SDL_Log("SDL_Init failed (%s)", SDL_GetError());
        return 1;
    }
    if (!SDL_ShaderCross_Init()) {
        SDL_Log("SDL_ShaderCross_Init failed (%s)", SDL_GetError());
        return 1;
    }
    SDL_ShaderCross_Quit();
    SDL_Quit();
    return 0;
}
