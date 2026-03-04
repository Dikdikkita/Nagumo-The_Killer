#define SDL_MAIN_HANDLED
#include "Engine.h"
#include <SDL.h>
#include <iostream>

int main(int argc, char* args[]) {
    SDL_SetMainReady();
    Engine engine;
    if (!engine.initialize()) {
        std::cerr << "Failed to initialize engine.\n";
        return -1;
    }

    engine.run();
    engine.shutdown();

    return 0;
}
