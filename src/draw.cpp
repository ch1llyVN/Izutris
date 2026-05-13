#include "tetris.h"

string Tetris::getWaveColor() {
    string colors[] = {"\x1B[91m", "\x1B[93m", "\x1B[92m", "\x1B[96m", "\x1B[94m", "\x1B[95m"};
    return colors[(waveFrame / 4) % 6];
}

string Tetris::getStr(int type, bool ghost, bool isClearing, int x, int y) {
    if (whiteFlashTimer > 0) {
        for (auto& cell : lastLockedCells) if (cell.first == x && cell.second == y) return "\x1B[" + cfg.flashColor + "m██\x1B[0m";
        if (isClearing && !useRainbowEffect) return "\x1B[" + cfg.clearColor + "m██\x1B[0m";
    }
    if (isClearing && useRainbowEffect) return getWaveColor() + "██" + "\x1B[0m";
    if (type == 0) {
        if (lightningTimer > 0 && lightningCols.count(x) && y <= lightningBottomY) return "\x1B[" + cfg.lightningColor + "m██\x1B[0m";
        if (x >= 0 && y >= 0 && (21 - y) < flameHeights[x]) return "\x1B[" + cfg.flameColor + "m██\x1B[0m"; 
        if (y >= 0 && y <= 1) return "  "; 
        return cfg.emptySkin;
    }
    string colors[] = {"", "\x1B[35m", "\x1B[33m", "\x1B[36m", "\x1B[31m", "\x1B[32m", "\x1B[34m", "\x1B[38;5;208m", "\x1B[97m"};
    if (ghost) {
        string gColor = cfg.ghostColored ? colors[type] : "\x1B[38;5;232m";
        string gSkin = (cfg.uniformSkin || type == 8) ? cfg.ghostSkin : cfg.pieceSkins[type];
        return gColor + gSkin + "\x1B[0m";
    }
    string s = (cfg.uniformSkin || type == 8) ? cfg.skin : cfg.pieceSkins[type];
    if (combo > 0 && x >= 0 && y >= 0) {
        try {
            int bgColorCode = stoi(cfg.flameColor) + 10; 
            return "\x1B[" + to_string(bgColorCode) + "m\x1B[" + cfg.comboClr + "m" + s + "\x1B[0m"; 
        } catch (...) {
            return "\x1B[" + cfg.comboClr + "m" + s + "\x1B[0m"; 
        }
    }
    return colors[type] + s + "\x1B[0m";
}

void Tetris::drawOverlay(string title, vector<string> options, string colorCode) {
    int cX = 20, cY = 10;
    string reset = "\x1B[0m";
    string fmt = "\x1B[1;97;" + colorCode + "m";

    auto printRow = [&](int y, string text, bool selected) {
        printf("\033[%d;%dH%s│ %s %-16s │%s", y, cX, fmt.c_str(), 
               selected ? "▶" : " ", text.c_str(), reset.c_str());
    };

    printf("\033[%d;%dH%s┌────────────────────┐%s", cY - 1, cX, fmt.c_str(), reset.c_str());
    printf("\033[%d;%dH%s│ %-18s │%s", cY, cX, fmt.c_str(), title.c_str(), reset.c_str());
    printf("\033[%d;%dH%s├────────────────────┤%s", cY + 1, cX, fmt.c_str(), reset.c_str());
    
    for (size_t i = 0; i < options.size(); i++) {
        printRow(cY + 2 + i, options[i], overlayCursor == i);
    }
    
    printf("\033[%d;%dH%s└────────────────────┘%s", cY + 2 + (int)options.size(), cX, fmt.c_str(), reset.c_str());
}

void Tetris::drawNameInput() {
    int cX = 21, cY = 10;
    string clr = "45"; 
    printf("\033[%d;%dH\x1B[1;97;%sm┌────────────────────┐\x1B[0m", cY - 1, cX, clr.c_str());
    printf("\033[%d;%dH\x1B[1;97;%sm│   NEW HIGH SCORE!  │\x1B[0m", cY, cX, clr.c_str());
    string inputRow = " Name: " + typingBuffer + "_";
    printf("\033[%d;%dH\x1B[1;97;%sm│ %-18s │\x1B[0m", cY + 1, cX, clr.c_str(), inputRow.c_str());
    printf("\033[%d;%dH\x1B[1;97;%sm│ [ENTER] to confirm │\x1B[0m", cY + 2, cX, clr.c_str());
    printf("\033[%d;%dH\x1B[1;97;%sm└────────────────────┘\x1B[0m", cY + 3, cX, clr.c_str());
}

void Tetris::drawKeystrokes() {
    struct KsEntry { string label; string key; };
    KsEntry entries[] = {
        {"HOLD", cfg.kHold}, {"CCW", cfg.kCCW}, {"180", cfg.k180}, {"CW", cfg.kCW},
        {"LEFT", cfg.kLeft}, {"SOFT", cfg.kSoft}, {"RIGHT", cfg.kRight}, {"HARD", cfg.kHard}
    };
    cout << "Keystrokes: ";
    for (auto& e : entries) {
        bool active = (keyStates.count(e.key) && keyStates[e.key] > 0);
        if (active) cout << "\x1B[" + cfg.keyColor + "m[" << e.label << "]\x1B[0m ";
        else cout << "[" << e.label << "] ";
    }
}

string Tetris::dVal(string v, int i) { return (isTyping && menuCursor == i) ? "\"" + typingBuffer + "_\"" : "\"" + v + "\""; }

string Tetris::bkt(string side, int idx) {
    bool flash = false;
    if (side == "<" && flashLeftTimer > 0 && menuCursor == idx) flash = true;
    if (side == ">" && flashRightTimer > 0 && menuCursor == idx) flash = true;
    if (flash) return "\x1B[1;93m" + side + "\x1B[0m";
    return side;
}

void Tetris::printMenuLine(string label, string value, int index, int width) {
    string displayStr = (value == "") ? label : label + ": " + value;
    int actualLen = 0;
    bool inEsc = false;
    for(size_t i=0; i < displayStr.length(); ++i) {
        if(displayStr[i] == '\x1B') inEsc = true;
        if(!inEsc) actualLen++;
        if(inEsc && displayStr[i] == 'm') inEsc = false;
    }
    printf("    │ ");
    if (menuCursor == index) {
        printf("\x1B[1;96m ▶ %s\x1B[0m", displayStr.c_str());
        for(int i=0; i < (width - 3 - actualLen); i++) printf(" ");
    } else {
        printf("   %s", displayStr.c_str());
        for(int i=0; i < (width - 3 - actualLen); i++) printf(" ");
    }
    printf(" │\n");
}

string Tetris::getRating() {
    if (multiplier >= 2.0f) return "\x1B[91mXtreme+  \x1B[0m"; 
    if (multiplier >= 1.8f) return "\x1B[38;5;208mS        \x1B[0m";
    if (multiplier >= 1.6f) return "\x1B[93mA        \x1B[0m"; 
    if (multiplier >= 1.4f) return "\x1B[92mB        \x1B[0m"; 
    if (multiplier >= 1.2f) return "\x1B[36mC        \x1B[0m"; 
    return "\x1B[96mD        \x1B[0m"; 
}

void Tetris::draw() {
    printf("\033[H\033[J"); 
    if (state == MAIN_MENU) {
        cout << "\x1B[96m";
        cout << " ___   _______  __   __  _______  ______    ___   _______ \n";
        cout << "|   | |       ||  | |  ||       ||    _ |  |   | |       |\n";
        cout << "|   | |____   ||  | |  ||_     _||   | ||  |   | |  _____|\n";
        cout << "|   |  ____|  ||  | |  |  |   |  |   |_||_ |   | | |_____ \n";
        cout << "|   | | ______||  |_|  |  |   |  |    __  ||   | |_____  |\n";
        cout << "|   | | |_____ |       |  |   |  |   |  | ||   |  _____| |\n";
        cout << "|___| |_______||_______|  |___|  |___|  |_||___| |_______|\n";
        cout << "               by Ch1lly\n\n";
        cout << "\x1B[0m";
        cout << "    ┌────────────────────────────┐\n";
        printMenuLine("Play", "", 0, 26);
        printMenuLine("Scoreboard", "", 1, 26);
        printMenuLine("Option", "", 2, 26);
        cout << "    │                            │\n";
        printMenuLine("Exit", "", 3, 26);
        cout << "    └────────────────────────────┘\n";
        cout << "    W/S UP/DOWN - Navigate | Enter/Space - Select\n";
    } 
    else if (state == SCOREBOARD) {
        cout << "\n\n    ┌── Scoreboard ────────────────────────────────┐\n";
        for(int i=0; i<(int)scores.size(); i++) printMenuLine(scores[i].toString(), "", i, 44);
        cout << "    │                                              │\n";
        printMenuLine("Back", "", (int)scores.size(), 44);
        cout << "    └──────────────────────────────────────────────┘\n";
        cout << "    W/S UP/DOWN - Navigate | Enter/Space - Select | M - Main Menu\n";
    }
    else if (state == OPTION_MENU) {
        cout << "\n\n    ┌── Option ────────┐\n";
        printMenuLine("Handling", "", 0, 16);
        printMenuLine("Visual", "", 1, 16);
        printMenuLine("Control", "", 2, 16);
        cout << "    │                  │\n";
        printMenuLine("Back", "", 3, 16);
        cout << "    └──────────────────┘\n";
        cout << "    W/S UP/DOWN - Navigate | Enter/Space - Select | M - Main Menu\n";
    } else if (state == HANDLING_MENU) {
        string sdfValue = (cfg.sdf == 0) ? "INF" : to_string(cfg.sdf) + " x";
        cout << "\n\n    ┌── Handling ────────────────────────────┐\n";
        printMenuLine("ARR", bkt("<", 0) + " " + to_string(cfg.arr) + " f " + bkt(">", 0), 0, 38);
        printMenuLine("DAS", bkt("<", 1) + " " + to_string(cfg.das) + " f " + bkt(">", 1), 1, 38);
        printMenuLine("SDF", bkt("<", 2) + " " + sdfValue + " " + bkt(">", 2), 2, 38);
        printMenuLine("DCD", bkt("<", 3) + " " + to_string(cfg.dcd) + " f " + bkt(">", 3), 3, 38);
        cout << "    │                                        │\n";
        printMenuLine("Back", "", 4, 38);
        cout << "    └────────────────────────────────────────┘\n";
        cout << "    W/S UP/DOWN - Navigate | A/D LEFT/RIGHT - Adjust | Enter/Space - Select | M - Main Menu\n";
    } else if (state == VISUAL_MENU) {
        cout << "    ┌── Visual ─────────────────┐\n";
        printMenuLine("Uniform skin", bkt("<", 0) + (cfg.uniformSkin ? " On " : " Off ") + bkt(">", 0), 0, 25);
        printMenuLine("Show Ghost", bkt("<", 1) + (cfg.showGhost ? " On " : " Off ") + bkt(">", 1), 1, 25);
        printMenuLine("Ghost Color", bkt("<", 2) + (cfg.ghostColored ? " On " : " Off ") + bkt(">", 2), 2, 25);
        printMenuLine("Spawn Preview", bkt("<", 3) + (cfg.showSpawnPreview ? " On " : " Off ") + bkt(">", 3), 3, 25);
        printMenuLine("General Skin", dVal(cfg.skin, 4), 4, 25);
        printMenuLine("T", dVal(cfg.pieceSkins[1], 5), 5, 25);
        printMenuLine("O", dVal(cfg.pieceSkins[2], 6), 6, 25);
        printMenuLine("I", dVal(cfg.pieceSkins[3], 7), 7, 25);
        printMenuLine("L", dVal(cfg.pieceSkins[7], 8), 8, 25);
        printMenuLine("J", dVal(cfg.pieceSkins[6], 9), 9, 25);
        printMenuLine("S", dVal(cfg.pieceSkins[4], 10), 10, 25);
        printMenuLine("Z", dVal(cfg.pieceSkins[5], 11), 11, 25);
        printMenuLine("Empty", dVal(cfg.emptySkin, 12), 12, 25);
        printMenuLine("Ghost Skin", dVal(cfg.ghostSkin, 13), 13, 25);
        printMenuLine("Clear Color", dVal(cfg.clearColor, 14), 14, 25);
        printMenuLine("Flame Color", dVal(cfg.flameColor, 15), 15, 25);
        printMenuLine("Flash Color", dVal(cfg.flashColor, 16), 16, 25);
        printMenuLine("Keystroke Color", dVal(cfg.keyColor, 17), 17, 25);
        printMenuLine("Lightning Color", dVal(cfg.lightningColor, 18), 18, 25);
        printMenuLine("Combo Color", dVal(cfg.comboClr, 19), 19, 25);
        printf("    │                           │\n");
        printMenuLine("Back", "", 20, 25);
        cout << "    └───────────────────────────┘\n";
        cout << "    W/S UP/DOWN - Navigate | A/D LEFT/RIGHT - Adjust | Enter/Space - Select | M - Main Menu\n";
    } else if (state == CONTROL_MENU) {
        cout << "    ┌── Control ──────────────────────┐\n";
        string* keys[] = {&cfg.kLeft, &cfg.kRight, &cfg.k180, &cfg.kCCW, &cfg.kCW, &cfg.kHold, &cfg.kSoft, &cfg.kHard, &cfg.kPause, &cfg.kRestart, &cfg.kMenu, &cfg.kLock};
        string labels[] = {"Left", "Right", "180", "CCW", "CW", "Hold", "Soft drop", "Hard drop", "Pause", "Restart", "Menu", "Train Lock"};
        for(int i=0; i<12; i++) printMenuLine(labels[i], dVal(*keys[i], i), i, 31);
        printf("    │                                 │\n");
        printMenuLine("Back", "", 12, 31);
        cout << "    └─────────────────────────────────┘\n";
        cout << "    W/S UP/DOWN - Navigate | A/D LEFT/RIGHT - Adjust | Enter/Space - Select | M - Main Menu\n";
    } else if (state == PLAY_MENU) {
        cout << "\n\n    ┌── Modes ─────────────────┐\n";
        printMenuLine("Zen", "", 0, 24);
        printMenuLine("Marathon", bkt("<", 1) + " " + to_string(marathonLimit) + " " + bkt(">", 1), 1, 24);
        printMenuLine("Blitz", bkt("<", 2) + " " + to_string(blitzTimeLimit / 60) + "m " + bkt(">", 2), 2, 24);
        printMenuLine("Training", "", 3, 24);
        cout << "    │                          │\n";
        printMenuLine("Back", "", 4, 24);
        cout << "    └──────────────────────────┘\n";
        cout << "    W/S UP/DOWN - Navigate | A/D LEFT/RIGHT - Adjust | Enter/Space - Select | M - Main Menu\n";
    } else if (state == INGAME || state == NAME_INPUT) {
        cout << "  "; drawKeystrokes(); cout << "\n";
        int gy = curY; 
        if (!isPaused && !gameOver && cfg.showGhost) 
            while (!collision(curX, gy + 1, curPiece)) gy++;
        
        int ts = (int)(activeTime / 1000);
        char statLine[16][32]; memset(statLine, 0, sizeof(statLine));
        int sl = 0;
        if (mode == BLITZ) {
            int rem = (int)blitzTimer;
            snprintf(statLine[sl++], 32, "Time: %02d:%02d", rem/60000, (rem/1000)%60);
        } else {
            snprintf(statLine[sl++], 32, "Time: %02d:%02d", (ts%3600)/60, ts%60);
        }
        if (mode == MARATHON) snprintf(statLine[sl++], 32, "Lines:%d/%d", linesClearedTotal, marathonLimit);
        else if (mode == BLITZ) snprintf(statLine[sl++], 32, "Lines: %d", linesClearedTotal);
        snprintf(statLine[sl++], 32, "Score: %lld", score);
        string rating = getRating();
        int barLen = 16;
        float decayFrac = 1.0f - (decayTimer / DECAY_INTERVAL);
        if (decayFrac < 0) decayFrac = 0; if (decayFrac > 1) decayFrac = 1;
        int filled = (int)(decayFrac * barLen);
        string decayBar = ""; for (int i = 0; i < filled; i++) decayBar += "█"; for (int i = filled; i < barLen; i++) decayBar += " ";
        char multBuf[24]; snprintf(multBuf, sizeof(multBuf), "x%.2f Mult", multiplier);
        string comboLine = ""; if (combo >= 1) comboLine = "+ Combo x" + to_string(combo + 1);
        string b2bLine = ""; if (b2b > 0) b2bLine = "+ B2B x" + to_string(b2b);
        string actLine = actionText; if (!actLine.empty()) { actLine = "+ " + actLine; if(actLine.size() > 16) actLine = actLine.substr(0, 16); }

        cout << "\n";

        cout << "  ┌ NEXT ────┐                            ┌ RATING ──────────┐\n";
        for (int y = 0; y < 22; y++) { 
            if (y < 15) {
                int qi = y / 3; int row = y % 3;
                if (row < 2 && qi < 5) {
                    Piece p = bag.getPiece(nextQueue[qi]); int minX = 9, minY = 9;
                    for(auto& c : p.cells) { minX = min(minX, c.first); minY = min(minY, c.second); }
                    cout << "  │ ";
                    for(int x = 0; x < 4; x++) {
                        bool occ = false; for(auto& c : p.cells) if(c.first-minX==x && c.second-minY==row) { occ=true; break; }
                        cout << (occ ? getStr(nextQueue[qi]) : "  ");
                    }
                    cout << " │";
                } else cout << "  │          │";
            } else if (y == 15) cout << "  ├── HOLD ──┤";
            else if (y < 18) {
                int hrow = y - 16; cout << "  │ ";
                if (holdPieceType) {
                    Piece p = bag.getPiece(holdPieceType); int minX = 9, minY = 9;
                    for(auto& c : p.cells) { minX = min(minX, c.first); minY = min(minY, c.second); }
                    for(int x = 0; x < 4; x++){
                        bool occ = false; for(auto& c : p.cells) if(c.first-minX==x && c.second-minY==hrow) { occ=true; break; }
                        cout << (occ ? getStr(holdPieceType) : "  ");
                    }
                } else cout << "        ";
                cout << " │";
            } else if (y == 18) cout << "  └──────────┘";
            else cout << "              ";

            if (y < 2) cout << "   "; else cout << "  │";
            bool isRowClearing = false; for(int ly : clearingLines) if(ly == y) isRowClearing = true;
            for (int x = 0; x < 10; x++) {
                if (isAnimating && isRowClearing) cout << getStr(0, false, true, x, y);
                else {
                    int pT = 0; bool isG = false;
                    if(!isPaused && !gameOver) {
                        for(auto& p : curPiece.cells) { 
                            if(curX+p.first==x && curY+p.second==y) pT = curPiece.type; 
                            if(cfg.showGhost && curX+p.first==x && gy+p.second==y && pT==0) { pT = curPiece.type; isG = true; } 
                        }
                        if (pT == 0 && cfg.showSpawnPreview && !nextQueue.empty()) {
                            Piece np = bag.getPiece(nextQueue.front());
                            for(auto& p : np.cells) {
                                if(4 + p.first == x && 1 + p.second == y) { pT = np.type; isG = true; }
                            }
                        }
                    }
                    cout << getStr(pT ? pT : board[y][x], isG, false, x, y);
                }
            }
            if (y < 2) cout << "   "; else cout << "│";

            {
                int pr = y;
                auto printRatingRow = [&](const string& content) {
                    int vlen = 0; bool inE = false;
                    for(char ch : content){ if(ch=='\x1B') inE=true; if(!inE) vlen++; if(inE&&ch=='m') inE=false; }
                    printf("    │ %-*s │", 16 + (int)(content.size() - vlen), content.c_str());
                };
                if      (pr == 0)  { printf("  │ Style: %-8s │", rating.c_str()); }
                else if (pr == 1)  { printf("  │  ─────────────── │"); }
                else if (pr == 2)  { printRatingRow(comboLine); }
                else if (pr == 3)  { printRatingRow(actLine); }
                else if (pr == 4)  { printRatingRow(b2bLine); }
                else if (pr == 5)  { printf("    │  ─────────────── │"); }
                else if (pr == 6)  { printRatingRow("Decay:"); }
                else if (pr == 7)  { printRatingRow(decayBar); }
                else if (pr == 8)  { printRatingRow(string(multBuf)); }
                else if (pr == 9)  { printf("    └──────────────────┘"); }
                else if (pr == 10) { printf("  %-18s", statLine[0]); }
                else if (pr == 11) { printf("  %-18s", sl > 1 ? statLine[1] : ""); }
                else if (pr == 12) { printf("  %-18s", sl > 2 ? statLine[2] : ""); }
                else               { printf("  %-18s", ""); }
            }
            cout << "\n";
        }
        string bar = "                └"; 
        if (isOnGround() && !gameOver && !isPaused && !isAnimating) { 
            int p = (int)((1.0f-(lockTimer/lockDelayLimit))*20); if(p<0) p=0; for(int i=0; i<20; i++) bar += (i<p?"─":" "); 
        }
        else bar += "────────────────────";
        cout << bar << "┘\n";
        cout << " \x1B[90mIzutris by Ch1lly~\x1B[0m\n";

        if (state == NAME_INPUT) drawNameInput();
        else if (gameOver) {
            string l1 = "   GAME OVER!   ";
            if (mode == MARATHON && linesClearedTotal >= marathonLimit) l1 = "  MARATHON WIN! ";
            else if (mode == BLITZ) l1 = " BLITZ FINISHED ";
            drawOverlay(l1, {"Retry", "Main Menu"}, "41");
        } else if (isPaused) {
            if (mode == TRAINING) {
                string trainNames[] = {"7-Bag", "I", "O", "T", "L", "J", "S", "Z", "1 Block"};
                string gravStr = "Grav: " + to_string((int)gravity); string pieceStr = "Piece: " + trainNames[trainingPieceSelect];
                drawOverlay("   TRAINING PAUSE ", {"Resume", gravStr, pieceStr, "Main Menu"}, "44");
            } else drawOverlay("     PAUSED     ", {"Resume", "Main Menu"}, "44");
        }
    }
    cout << flush;
}
