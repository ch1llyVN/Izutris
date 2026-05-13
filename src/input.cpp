#include "tetris.h"

void Tetris::handleInput() {
    char c; string k = "";
    if (read(STDIN_FILENO, &c, 1) > 0) {
        if (isTyping) {
            if (c == '\n' || c == '\r') {
                if(state == NAME_INPUT) { 
                    saveScore(typingBuffer == "" ? "GUEST" : typingBuffer); 
                    state = SCOREBOARD; 
                    menuCursor = (int)scores.size(); 
                }
                else finalizeTyping();
                isTyping = false; return;
            }
            if (c == 127 || c == 8) { if(!typingBuffer.empty()) typingBuffer.pop_back(); return; }
            if (typingBuffer.length() < 10) typingBuffer += c; return;
        }
        if (c == 27) {
            char nC; if (read(STDIN_FILENO, &nC, 1) <= 0) k = "ESC";
            else if (nC == '[') {
                char seq; if (read(STDIN_FILENO, &seq, 1) > 0) {
                    if (seq == 'A') k = "UP"; else if (seq == 'B') k = "DOWN";
                    else if (seq == 'C') k = "RIGHT"; else if (seq == 'D') k = "LEFT";
                }
            }
        } else { k = string(1, c); if(c == ' ') k = " "; }
        
        keyStates[k] = 150.0f;
        if (k == "LEFT") flashLeftTimer = 100.0f;
        if (k == "RIGHT") flashRightTimer = 100.0f;

        if (state == MAIN_MENU) handleMainMenu(k);
        else if (state == PLAY_MENU) handlePlayMenu(k);
        else if (state == OPTION_MENU) handleOptionMenu(k);
        else if (state == HANDLING_MENU) handleHandlingMenu(k);
        else if (state == VISUAL_MENU) handleVisualMenu(k);
        else if (state == CONTROL_MENU) handleControlMenu(k);
        else if (state == SCOREBOARD) handleScoreboardMenu(k);
        else if (state == INGAME) handleIngameInput(k);
    } else { lastDirKey = ""; }
}

void Tetris::finalizeTyping() {
    if (state == VISUAL_MENU) {
        if (menuCursor == 4) cfg.skin = typingBuffer;
        else if (menuCursor >= 5 && menuCursor <= 11) {
            int mapIdx[] = {1, 2, 3, 7, 6, 4, 5}; 
            cfg.pieceSkins[mapIdx[menuCursor-5]] = typingBuffer;
        }
        else if (menuCursor == 12) cfg.emptySkin = typingBuffer;
        else if (menuCursor == 13) cfg.ghostSkin = typingBuffer;
        else if (menuCursor == 14) cfg.clearColor = typingBuffer;
        else if (menuCursor == 15) cfg.flameColor = typingBuffer;
        else if (menuCursor == 16) cfg.flashColor = typingBuffer;
        else if (menuCursor == 17) cfg.keyColor = typingBuffer;
        else if (menuCursor == 18) cfg.lightningColor = typingBuffer;
        else if (menuCursor == 19) cfg.comboClr = typingBuffer;
    } else if (state == CONTROL_MENU) {
        string* targets[] = {&cfg.kLeft, &cfg.kRight, &cfg.k180, &cfg.kCCW, &cfg.kCW, &cfg.kHold, &cfg.kSoft, &cfg.kHard, &cfg.kPause, &cfg.kRestart, &cfg.kMenu, &cfg.kLock};
        if (menuCursor <= 11) *targets[menuCursor] = typingBuffer;
    }
    cfg.save("setting.cfg");
}

void Tetris::handleMainMenu(string k) {
    if (k == "UP" || k == "w") menuCursor = (menuCursor - 1 + 4) % 4;
    else if (k == "DOWN" || k == "s") menuCursor = (menuCursor + 1) % 4;
    else if (k == " " || k == "\n" || k == "e") {
        if (menuCursor == 0) state = PLAY_MENU;
        else if (menuCursor == 1) { state = SCOREBOARD; menuCursor = (int)scores.size(); }
        else if (menuCursor == 2) state = OPTION_MENU;
        else if (menuCursor == 3) exit(0);
        if (state != SCOREBOARD) menuCursor = 0;
    }
}

void Tetris::handleScoreboardMenu(string k) {
    int sz = (int)scores.size();
    if (k == "UP" || k == "w") menuCursor = (menuCursor - 1 + sz + 1) % (sz + 1);
    else if (k == "DOWN" || k == "s") menuCursor = (menuCursor + 1) % (sz + 1);
    else if (k == " " || k == "\n" || k == "e") {
        if(menuCursor == sz) { state = MAIN_MENU; menuCursor = 0; }
    }
}

void Tetris::handlePlayMenu(string k) {
    if (k == "UP" || k == "w") menuCursor = (menuCursor - 1 + 5) % 5;
    else if (k == "DOWN" || k == "s") menuCursor = (menuCursor + 1) % 5;
    else if (k == "LEFT" || k == "a") {
        if (menuCursor == 1) marathonLimit = min(200, marathonLimit + 10); 
        if (menuCursor == 2) blitzTimeLimit = min(600, blitzTimeLimit + 30);
        flashLeftTimer = 100.0f;
    } else if (k == "RIGHT" || k == "d") {
        if (menuCursor == 1) marathonLimit = max(10, marathonLimit - 10);
        if (menuCursor == 2) blitzTimeLimit = max(60, blitzTimeLimit - 30);
        flashRightTimer = 100.0f;
    } else if (k == " " || k == "\n" || k == "e") {
        if (menuCursor == 0) { mode = ZEN; state = INGAME; initGame(); }
        else if (menuCursor == 1) { mode = MARATHON; state = INGAME; initGame(); }
        else if (menuCursor == 2) { mode = BLITZ; state = INGAME; initGame(); }
        else if (menuCursor == 3) { mode = TRAINING; state = INGAME; initGame(); }
        else { state = MAIN_MENU; menuCursor = 0; }
    }
}

void Tetris::handleOptionMenu(string k) {
    if (k == "UP" || k == "w") menuCursor = (menuCursor - 1 + 4) % 4;
    else if (k == "DOWN" || k == "s") menuCursor = (menuCursor + 1) % 4;
    else if (k == " " || k == "\n" || k == "e") {
        if (menuCursor == 0) state = HANDLING_MENU;
        else if (menuCursor == 1) state = VISUAL_MENU;
        else if (menuCursor == 2) state = CONTROL_MENU;
        else { state = MAIN_MENU; cfg.save("setting.cfg"); }
        menuCursor = 0;
    }
}

void Tetris::handleHandlingMenu(string k) {
    if (k == "UP" || k == "w") menuCursor = (menuCursor - 1 + 5) % 5;
    else if (k == "DOWN" || k == "s") menuCursor = (menuCursor + 1) % 5;
    else if (k == "LEFT" || k == "a") {
        if(menuCursor == 0) cfg.arr++;
        else if(menuCursor == 1) cfg.das++;
        else if(menuCursor == 2) { if(cfg.sdf < 100) cfg.sdf++; } 
        else if(menuCursor == 3) cfg.dcd++;
        flashLeftTimer = 100.0f;
    } else if (k == "RIGHT" || k == "d") {
        if(menuCursor == 0) cfg.arr = max(0, cfg.arr - 1);
        else if(menuCursor == 1) cfg.das = max(0, cfg.das - 1);
        else if(menuCursor == 2) cfg.sdf = max(0, cfg.sdf - 1); 
        else if(menuCursor == 3) cfg.dcd = max(0, cfg.dcd - 1);
        flashRightTimer = 100.0f;
    } else if (k == " " || k == "\n" || k == "e") if(menuCursor == 4) { state = OPTION_MENU; cfg.save("setting.cfg"); menuCursor = 0; }
}

void Tetris::handleVisualMenu(string k) {
    if (k == "UP" || k == "w") menuCursor = (menuCursor - 1 + 21) % 21;
    else if (k == "DOWN" || k == "s") menuCursor = (menuCursor + 1) % 21;
    else if (k == "LEFT" || k == "a" || k == "RIGHT" || k == "d") {
        if (menuCursor == 0) cfg.uniformSkin = !cfg.uniformSkin;
        else if (menuCursor == 1) cfg.showGhost = !cfg.showGhost;
        else if (menuCursor == 2) cfg.ghostColored = !cfg.ghostColored;
        else if (menuCursor == 3) cfg.showSpawnPreview = !cfg.showSpawnPreview; 
        if(k == "LEFT" || k == "a") flashLeftTimer = 100.0f; else flashRightTimer = 100.0f;
    } else if (k == " " || k == "\n" || k == "e") {
        if (menuCursor >= 4 && menuCursor <= 19) { isTyping = true; typingBuffer = ""; }
        else if (menuCursor == 20) { state = OPTION_MENU; cfg.save("setting.cfg"); menuCursor = 0; }
    }
}

void Tetris::handleControlMenu(string k) {
    if (k == "UP" || k == "w") menuCursor = (menuCursor - 1 + 13) % 13;
    else if (k == "DOWN" || k == "s") menuCursor = (menuCursor + 1) % 13;
    else if (k == " " || k == "\n" || k == "e") {
        if (menuCursor <= 11) { isTyping = true; typingBuffer = ""; }
        else { state = OPTION_MENU; cfg.save("setting.cfg"); menuCursor = 0; }
    }
}

void Tetris::handleIngameInput(string k) {
    if (curY <= 1) {
        if (k == cfg.kHold) queuedHold = true;
        else if (k == cfg.kCW) queuedRotate = 1;
        else if (k == cfg.kCCW) queuedRotate = 2;
        else if (k == cfg.k180) queuedRotate = 3;
    }
    if (k == cfg.kRestart) { initGame(); return; }
    if (k == cfg.kPause || k == "ESC") { if (!gameOver) isPaused = !isPaused; overlayCursor = 0; return; }
    
    if (isPaused || gameOver) {
        int maxOverlay = (isPaused && mode == TRAINING) ? 4 : 2;
        if (k == "UP" || k == "w") overlayCursor = (overlayCursor - 1 + maxOverlay) % maxOverlay;
        else if (k == "DOWN" || k == "s") overlayCursor = (overlayCursor + 1) % maxOverlay;
        else if (isPaused && mode == TRAINING && (k == "LEFT" || k == "a" || k == "RIGHT" || k == "d")) {
            if (overlayCursor == 1) { 
                if (k == "LEFT" || k == "a") gravity = max(0.0f, gravity - 50.0f);
                else gravity = min(5000.0f, gravity + 50.0f);
            } else if (overlayCursor == 2) { 
                if (k == "LEFT" || k == "a") trainingPieceSelect = (trainingPieceSelect - 1 + 9) % 9;
                else trainingPieceSelect = (trainingPieceSelect + 1) % 9;
                
                int trainTypes[] = {0, 3, 2, 1, 7, 6, 4, 5, 8};
                int selectedType = trainTypes[trainingPieceSelect];
                nextQueue.clear();
                if (trainingPieceSelect == 0) {
                    bag.reset();
                    for(int i=0; i<10; i++) nextQueue.push_back(bag.nextType());
                } else {
                    for(int i=0; i<10; i++) nextQueue.push_back(selectedType);
                    curPiece = bag.getPiece(selectedType);
                    if (collision(curX, curY, curPiece)) { curX = 4; curY = 0; }
                    lockTimer = 0;
                }
            }
        }
        else if (k == " " || k == "\n" || k == "e") {
            if (isPaused) {
                if (mode == TRAINING) {
                    if (overlayCursor == 0) isPaused = false;
                    else if (overlayCursor == 3) { state = MAIN_MENU; menuCursor = 0; isPaused = false; }
                } else {
                    if (overlayCursor == 0) isPaused = false;
                    else { state = MAIN_MENU; menuCursor = 0; isPaused = false; }
                }
            } else if (gameOver) {
                if (overlayCursor == 0) initGame();
                else { state = MAIN_MENU; menuCursor = 0; }
            }
        }
        else if (k == cfg.kMenu || k == "m") { state = MAIN_MENU; menuCursor = 0; }
        return;
    }

    if (gameOver || isPaused || isAnimating) return;

    if (mode == TRAINING && k == cfg.kLock) { lock(); return; } 
    else if (k == cfg.kLeft || k == cfg.kRight) { 
        if (k != lastDirKey) { move(k == cfg.kLeft ? -1 : 1); dasTimer = 0; arrTimer = 0; lastDirKey = k; } 
    }
    else if (k == cfg.kSoft) { moveDown(); lastMoveWasRotate = false; }
    else if (k == cfg.kCW) { rotate(1); dasTimer = max(0.0f, dasTimer - (float)cfg.dcd * 16.67f); }
    else if (k == cfg.kCCW) { rotate(2); dasTimer = max(0.0f, dasTimer - (float)cfg.dcd * 16.67f); }
    else if (k == cfg.k180) { rotate(3); dasTimer = max(0.0f, dasTimer - (float)cfg.dcd * 16.67f); }
    else if (k == cfg.kHold) hold();
    else if (k == cfg.kHard) { hardDrop(); lastMoveWasRotate = false; }
}
