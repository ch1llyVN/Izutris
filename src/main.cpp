#include <iostream>
#include <chrono>
#include <thread>
#include "tetris.h"
using namespace std;

int main() {
    printf("\033[2J\033[H\033[?25l"); 
    Tetris game; auto last = chrono::steady_clock::now(); 
    while (true) {
        auto now = chrono::steady_clock::now();
        float delta = chrono::duration<float, milli>(now - last).count();
        last = now; game.handleInput(); game.update(delta); game.draw();
        this_thread::sleep_for(chrono::milliseconds(16));
    }
    return 0;
}
