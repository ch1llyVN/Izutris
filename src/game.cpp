#include "tetris.h"

void Tetris::setNonBlocking() {
    struct termios ttystate; tcgetattr(STDIN_FILENO, &ttystate);
    ttystate.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

void Tetris::loadScores() {
    scores.clear();
    ifstream file("hiscore.sav");
    string line;
    while(getline(file, line)) {
        stringstream ss(line);
        string n, m, d, r;
        if (getline(ss, n, '|') && getline(ss, m, '|') && getline(ss, d, '|') && getline(ss, r, '|')) {
            auto trim = [](string s) {
                size_t first = s.find_first_not_of(' ');
                if (string::npos == first) return s;
                size_t last = s.find_last_not_of(' ');
                return s.substr(first, (last - first + 1));
            };
            scores.push_back({trim(n), trim(m), trim(d), trim(r)});
        }
    }
}

void Tetris::saveScore(string name) {
    if (mode == TRAINING) return;
    ScoreEntry e; e.name = name;
    if(mode == MARATHON) {
        e.mode = "Marathon"; e.detail = to_string(marathonLimit) + "L";
        int ts = (int)(activeTime / 1000);
        char buf[16]; sprintf(buf, "%d:%02d:%02d", ts/3600, (ts%3600)/60, ts%60);
        e.result = buf;
    } else {
        e.mode = "Blitz"; e.detail = to_string(blitzTimeLimit/60) + "m";
        e.result = to_string(score);
    }
    scores.insert(scores.begin(), e);
    if(scores.size() > 10) scores.pop_back();
    ofstream file("hiscore.sav");
    for(auto& s : scores) file << s.name << " | " << s.mode << " | " << s.detail << " | " << s.result << "\n";
}

void Tetris::initGame() {
    for(int y=0; y<22; y++) for(int x=0; x<10; x++) board[y][x] = 0;
    nextQueue.clear(); bag.reset();
    int trainTypes[] = {0, 3, 2, 1, 7, 6, 4, 5, 8};
    if (mode == TRAINING && trainingPieceSelect != 0) {
        for(int i=0; i<10; i++) nextQueue.push_back(trainTypes[trainingPieceSelect]);
    } else {
        for(int i=0; i<10; i++) nextQueue.push_back(bag.nextType());
    }
    holdPieceType = 0; score = 0; combo = -1; b2b = -1; actionText = "";
    multiplier = 1.0f; decayTimer = 0.0f;
    isAnimating = false; isPaused = false; gameOver = false;
    linesClearedTotal = 0; lightningTimer = 0; activeTime = 0;
    blitzTimer = (float)blitzTimeLimit * 1000.0f;
    spawnPiece();
}

SpinInfo Tetris::checkSpin() {
    SpinInfo result;

    if (!lastMoveWasRotate)
        return result;

    bool leftBlocked = collision(curX - 1, curY, curPiece);
    bool rightBlocked = collision(curX + 1, curY, curPiece);
    bool downBlocked = collision(curX, curY + 1, curPiece);

    bool immobile = leftBlocked && rightBlocked && downBlocked;
    bool kickSpin = lastRotationUsedKick && lastKickIndex >= 1;

    if (!immobile && !kickSpin)
        return result;

    result.valid = true;
    result.piece = curPiece.type;
    result.usedKick = lastRotationUsedKick;
    result.kickIndex = lastKickIndex;

    if (curPiece.type == 1) {
        int corners = 0;
        int dx[] = {-1, 1, -1, 1};
        int dy[] = {-1, -1, 1, 1};

        for (int i = 0; i < 4; i++) {
            int tx = curX + dx[i];
            int ty = curY + dy[i];

            if (tx < 0 || tx >= 10 || ty >= 22 || (ty >= 0 && board[ty][tx] != 0))
                corners++;
        }

        if (corners < 3)
            result.valid = false;

        if (corners == 3 && !lastRotationUsedKick)
            result.mini = true;
    }

    return result;
}

void Tetris::spawnPiece() {
    curPiece = bag.getPiece(nextQueue.front());
    nextQueue.pop_front();
    if (nextQueue.size() < 7) {
        int trainTypes[] = {0, 3, 2, 1, 7, 6, 4, 5, 8};
        if (mode == TRAINING && trainingPieceSelect != 0) {
            nextQueue.push_back(trainTypes[trainingPieceSelect]);
        } else {
            nextQueue.push_back(bag.nextType());
        }
    }
    curX = 4; curY = 1; 
    canHold = true; lockTimer = 0;
    lastMoveWasRotate = false;
    lastRotationUsedKick = false;
    lastKickIndex = 0;
    if (collision(curX, curY, curPiece)) {
        gameOver = true;
        if(mode != ZEN && mode != TRAINING) { state = NAME_INPUT; isTyping = true; typingBuffer = ""; }
    }
}

bool Tetris::collision(int nx, int ny, Piece p) {
    for (auto& cell : p.cells) {
        int tx = nx + cell.first, ty = ny + cell.second;
        if (tx < 0 || tx >= 10 || ty >= 22 || (ty >= 0 && board[ty][tx] != 0)) return true;
    }
    return false;
}

void Tetris::move(int dx) { if (!collision(curX + dx, curY, curPiece)) { curX += dx; lastMoveWasRotate = false; } }
void Tetris::moveDown() { if (!collision(curX, curY + 1, curPiece)) { curY++; score += 1; resetDecay(); } }
bool Tetris::isOnGround() { return collision(curX, curY + 1, curPiece); }

vector<pair<int,int>> Tetris::getIShape(int state) {
    switch(state) {
        case 0: return {{-1, 0}, { 0, 0}, { 1, 0}, { 2, 0}}; 
        case 1: return {{ 1,-1}, { 1, 0}, { 1, 1}, { 1, 2}}; 
        case 2: return {{-1, 1}, { 0, 1}, { 1, 1}, { 2, 1}}; 
        default: return {{ 0,-1}, { 0, 0}, { 0, 1}, { 0, 2}}; 
    }
}

void Tetris::rotate(int r_mode) {
    if (curPiece.type == 2 || curPiece.type == 8) return; 

    int oldState = curPiece.rotationState;
    int newState;
    if (r_mode == 1)      newState = (oldState + 1) % 4; // CW
    else if (r_mode == 2) newState = (oldState + 3) % 4; // CCW
    else                  newState = (oldState + 2) % 4; // 180

    Piece nS = curPiece;
    nS.rotationState = newState;

    if (curPiece.type == 3) {
        nS.cells = getIShape(newState);
    } else {
        int times = (r_mode == 3) ? 2 : (r_mode == 1 ? 1 : 3);
        for(int t=0; t<times; t++) {
            for (auto& p : nS.cells) {
                int x = p.first, y = p.second;
                p.first = -y; p.second = x; 
            }
        }
    }

    int lookupKey = oldState * 10 + newState;
    vector<pair<int, int>> kicks;
    if (curPiece.type == 3) kicks = srsOffsetsI[lookupKey];
    else kicks = srsOffsets[lookupKey];

    if (kicks.empty()) kicks.push_back({0, 0});

    for (int i = 0; i < (int)kicks.size(); i++) {
        auto offset = kicks[i];
        int dx = offset.first;
        int dy = -offset.second; 

        if (!collision(curX + dx, curY + dy, nS)) {
            curX += dx;
            curY += dy;
            curPiece = nS;
            lastMoveWasRotate = true;
            lastRotationUsedKick = (offset.first != 0 || offset.second != 0);
            lastKickIndex = i;
            lastRotationMode = r_mode;
            return;
        }
    }
}

void Tetris::hold() {
    if (!canHold) return;
    int oldType = curPiece.type;
    if (holdPieceType == 0) { 
        holdPieceType = oldType; 
        spawnPiece();
    } else { 
        int nextType = holdPieceType;
        holdPieceType = oldType; 
        curPiece = bag.getPiece(nextType);
        curX = 4; curY = 0; 
    }
    canHold = false; 
    lastMoveWasRotate = false;
    lockTimer = 0; fallTimer = 0;
}

void Tetris::resetDecay() { decayTimer = 0.0f; }

void Tetris::hardDrop() { 
    int dropped = 0;
    while (!isOnGround()) { curY++; dropped++; }
    score += dropped * 10;
    resetDecay();
    lightningCols.clear();
    int maxY = 0;
    for (auto& p : curPiece.cells) { lightningCols.insert(curX + p.first); if (curY + p.second > maxY) maxY = curY + p.second; }
    lightningBottomY = maxY; lightningTimer = 120.0f; whiteFlashTimer = 200.0f;
    lock(); 
}

void Tetris::lock() {
    SpinInfo spin = checkSpin();
    actionText = ""; 
    lastLockedCells.clear();
    for (auto& p : curPiece.cells) {
        int tx = curX + p.first, ty = curY + p.second;
        if (ty >= 0 && ty < 22) { board[ty][tx] = curPiece.type; lastLockedCells.push_back({tx, ty}); }
    }

    for (int x = 0; x < 10; x++) {
        if (board[0][x] != 0) { 
            gameOver = true;
            if(mode != ZEN && mode != TRAINING) { state = NAME_INPUT; isTyping = true; typingBuffer = ""; }
            return;
        }
    }

    clearingLines.clear();
    for (int y = 0; y < 22; y++) {
        bool full = true;
        for (int x = 0; x < 10; x++) if (board[y][x] == 0) full = false;
        if (full) clearingLines.push_back(y);
    }
    int lines = (int)clearingLines.size();
    useRainbowEffect = false;
    spin.lines = lines;
    lastSpin = spin;

    if (lines > 0 || spin.valid) {
        isAnimating = (lines > 0); animationTimer = 400.0f;
        bool isB2Bworthy = false;
        bool isPerfectClear = false;
        if (lines > 0) {
            set<int> clearSet(clearingLines.begin(), clearingLines.end());
            bool boardEmpty = true;
            for (int cy = 0; cy < 22 && boardEmpty; cy++) {
                if (clearSet.count(cy)) continue;
                for (int cx = 0; cx < 10 && boardEmpty; cx++)
                    if (board[cy][cx] != 0) boardEmpty = false;
            }
            isPerfectClear = boardEmpty;
        }
        long long baseScore = 0;
        if (spin.valid) {
            isB2Bworthy = (lines > 0);

            string spinName;

            switch(curPiece.type) {
                case 1: spinName = "T"; break;
                case 2: spinName = "O"; break;
                case 3: spinName = "I"; break;
                case 4: spinName = "S"; break;
                case 5: spinName = "Z"; break;
                case 6: spinName = "J"; break;
                case 7: spinName = "L"; break;
                default: spinName = "SPIN"; break;
            }

            if (lines == 0) {
                actionText = spinName + "-SPIN MINI";
                baseScore = 50;
            }
            else if (lines == 1) {
                actionText = spinName + "-SPIN SINGLE";
                baseScore = 600;
                useRainbowEffect = true;
            }
            else if (lines == 2) {
                actionText = spinName + "-SPIN DOUBLE";
                baseScore = 1000;
                useRainbowEffect = true;
            }
            else if (lines == 3) {
                actionText = spinName + "-SPIN TRIPLE";
                baseScore = 1250;
                useRainbowEffect = true;
            }
            else if (lines >= 4) {
                actionText = spinName + "-SPIN 4+";
                baseScore = 1500;
                useRainbowEffect = true;
            }
        } else {
            isB2Bworthy = (lines == 4);
            if      (lines == 1) { actionText = "SINGLE";  baseScore = 100; }
            else if (lines == 2) { actionText = "DOUBLE";  baseScore = 200; }
            else if (lines == 3) { actionText = "TRIPLE";  baseScore = 500; }
            else if (lines == 4) { actionText = "QUAD!!!"; baseScore = 1750; useRainbowEffect = true; }
        }
        if (isPerfectClear) { actionText = "CLEAR!!!"; baseScore = 1750; useRainbowEffect = true; }
        if (lines > 0) { combo++; linesClearedTotal += lines; } else combo = -1;
        if (isB2Bworthy) b2b++;
        else if (lines > 0) b2b = -1;
        if (combo >= 1) multiplier += 0.05f;
        if (b2b > 0)    multiplier += 0.05f;
        float totalMult = multiplier;
        if (totalMult < 1.0f) totalMult = 1.0f;
        score += (long long)(baseScore * totalMult);
        multiplier += 0.01f;
        resetDecay();
        if (!isAnimating) spawnPiece();
    } else { combo = -1; spawnPiece(); }
    
    if (mode == MARATHON && linesClearedTotal >= marathonLimit) {
        gameOver = true; state = NAME_INPUT; isTyping = true; typingBuffer = "";
    }
}

void Tetris::update(float dt) {
    if (flashLeftTimer > 0) flashLeftTimer -= dt;
    if (flashRightTimer > 0) flashRightTimer -= dt;
    for (auto& kv : keyStates) if (kv.second > 0) kv.second -= dt;
    if (state == INGAME && !isPaused && !gameOver) activeTime += dt;
    if (state != INGAME || gameOver || isPaused) return;
    waveFrame++;
    if (lightningTimer > 0) lightningTimer -= dt;
    if (whiteFlashTimer > 0) whiteFlashTimer -= dt;
    decayTimer += dt;
    while (decayTimer >= DECAY_INTERVAL) {
        decayTimer -= DECAY_INTERVAL;
        multiplier -= DECAY_AMOUNT;
        if (multiplier < MULTIPLIER_MIN) multiplier = MULTIPLIER_MIN;
    }

    float dasMs = (float)cfg.das * 16.6667f;
    float arrMs = (float)cfg.arr * 16.6667f;

    if (lastDirKey != "") {
        dasTimer += dt;
        if (dasTimer >= dasMs) {
            arrTimer += dt;
            if (cfg.arr == 0) { 
                while(!collision(curX + (lastDirKey == cfg.kLeft ? -1 : 1), curY, curPiece)) 
                    move(lastDirKey == cfg.kLeft ? -1 : 1); 
            }
            else while (arrTimer >= arrMs) { 
                move(lastDirKey == cfg.kLeft ? -1 : 1); 
                arrTimer -= arrMs; 
            }
        }
    }

    if (combo > 0) {
        for (int i = 0; i < 10; i++) {
            int target = 3 + (combo * 2); if (target > 18) target = 18;
            flameHeights[i] = (int)(flameRng() % 4) + target; 
        }
    } else for (int i = 0; i < 10; i++) if (flameHeights[i] > 0) flameHeights[i]--;

    if (mode == BLITZ) { 
        blitzTimer -= dt; 
        if (blitzTimer <= 0) { 
            blitzTimer = 0; gameOver = true; 
            state = NAME_INPUT; isTyping = true; typingBuffer = ""; 
        } 
    }

    if (isAnimating) {
        animationTimer -= dt;
        if (animationTimer <= 0) {
            for (int y : clearingLines) {
                for (int ty = y; ty > 0; ty--) for (int tx = 0; tx < 10; tx++) board[ty][tx] = board[ty-1][tx];
                for (int tx = 0; tx < 10; tx++) board[0][tx] = 0;
            }
            isAnimating = false; clearingLines.clear(); spawnPiece();
        }
        return; 
    }

    if (isOnGround()) { lockTimer += dt; if (lockTimer >= lockDelayLimit) lock(); }
    else { 
        lockTimer = 0; 
        fallTimer += dt; 
        float effectiveGravity = gravity;
        if (keyStates[cfg.kSoft] > 0) {
            if (cfg.sdf == 0) effectiveGravity = 0.001f; 
            else effectiveGravity = gravity / (float)cfg.sdf;
        }

        if (fallTimer >= effectiveGravity) { 
            curY++; fallTimer = 0; 
            lastMoveWasRotate = false; 
        } 
    }
}
