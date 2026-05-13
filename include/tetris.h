#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <deque>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <sstream>
#include <set>
#include <map>
#include <cstring>
#include "types.h"
#include "config.h"
#include "piece.h"
#include "score_entry.h"
using namespace std;

struct SpinInfo {
    bool valid = false;
    bool mini = false;
    int lines = 0;
    int piece = 0;
    bool usedKick = false;
    int kickIndex = 0;
};

class Tetris {
    int board[22][10]; 
    int curX, curY;
    Piece curPiece;
    deque<int> nextQueue;
    int holdPieceType;
    bool canHold;
    SevenBag bag;
    Config cfg;
    float gravity = 800.0f, fallTimer = 0, lockDelayLimit = 1000.0f, lockTimer = 0;
    long long score = 0;
    int combo = -1;
    int b2b = -1;
    string actionText = "";
    bool lastMoveWasRotate = false; 
    bool lastRotationUsedKick = false;
    int lastKickIndex = 0;
    int lastRotationMode = 0;
    SpinInfo lastSpin;
    bool queuedHold = false;
    int queuedRotate = 0;
    float activeTime = 0;
    float multiplier = 1.0f;
    float decayTimer = 0.0f;
    static constexpr float DECAY_INTERVAL = 5000.0f;
    static constexpr float DECAY_AMOUNT   = 0.5f;
    static constexpr float MULTIPLIER_MIN = 1.0f;
    bool isAnimating = false;
    vector<int> clearingLines;
    float animationTimer = 0;
    int waveFrame = 0;
    float whiteFlashTimer = 0;
    vector<pair<int, int>> lastLockedCells;
    bool useRainbowEffect = false;
    int flameHeights[10] = {0};
    mt19937 flameRng;
    float lightningTimer = 0;
    set<int> lightningCols;
    int lightningBottomY = 0;
    GameState state = MAIN_MENU;
    GameMode mode = ZEN;
    int menuCursor = 0;
    int overlayCursor = 0; 
    bool isTyping = false;
    string typingBuffer = "";
    map<string, float> keyStates;
    string lastDirKey = "";
    float dasTimer = 0, arrTimer = 0;
    int marathonLimit = 40, blitzTimeLimit = 120;
    float blitzTimer = 0;
    float flashLeftTimer = 0, flashRightTimer = 0;
    int linesClearedTotal = 0;
    int trainingPieceSelect = 0; 
    vector<ScoreEntry> scores;
    map<int, vector<pair<int, int>>> srsOffsets = {
        {01, {{0,0}, {-1,0}, {-1, 1}, {0,-2}, {-1,-2}}}, {10, {{0,0}, {1,0}, {1,-1}, {0,2}, {1,2}}},
        {12, {{0,0}, {1,0}, {1,-1}, {0,2}, {1,2}}},     {21, {{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}}},
        {23, {{0,0}, {1,0}, {1,1}, {0,-2}, {1,-2}}},     {32, {{0,0}, {-1,0}, {-1,-1}, {0,2}, {-1,2}}},
        {30, {{0,0}, {-1,0}, {-1,-1}, {0,2}, {-1,2}}},   {03, {{0,0}, {1,0}, {1,1}, {0,-2}, {1,-2}}},
        {02, {{0,0}, {0,1}, {1,1}, {-1,1}, {1,0}, {-1,0}, {0,-1}}}, {20, {{0,0}, {0,-1}, {-1,-1}, {1,-1}, {-1,0}, {1,0}, {0,1}}},
        {13, {{0,0}, {1,0}, {1,2}, {1,1}, {0,2}, {0,1}, {1,-1}}},   {31, {{0,0}, {-1,0}, {-1,2}, {-1,1}, {0,2}, {0,1}, {-1,-1}}}
    };
    map<int, vector<pair<int, int>>> srsOffsetsI = {
        {01, {{0,0}, {-2,0}, {1,0}, {-2,-1}, {1,2}}},  {10, {{0,0}, {2,0}, {-1,0}, {2,1}, {-1,-2}}},
        {12, {{0,0}, {-1,0}, {2,0}, {-1,2}, {2,-1}}},  {21, {{0,0}, {1,0}, {-2,0}, {1,-2}, {-2,1}}},
        {23, {{0,0}, {2,0}, {-1,0}, {2,1}, {-1,-2}}},  {32, {{0,0}, {-2,0}, {1,0}, {-2,-1}, {1,2}}},
        {30, {{0,0}, {1,0}, {-2,0}, {1,-2}, {-2,1}}},  {03, {{0,0}, {-1,0}, {2,0}, {-1,2}, {2,-1}}},
        {02, {{0,1}, {0,2}, {-1,1}, {1,1}}}, {20, {{0,-1}, {0,-2}, {1,-1}, {-1,-1}}},
        {13, {{-1,0}, {-2,0}, {-1,1}, {-1,-1}}}, {31, {{1,0}, {2,0}, {1,-1}, {1,1}}}
    };

public:
    Tetris() : flameRng(random_device{}()) { 
        cfg.load("setting.cfg");
        loadScores();
        setNonBlocking(); 
        initGame(); 
    }

    void setNonBlocking();
    void loadScores();
    void saveScore(string name);
    void initGame();

    bool gameOver = false, isPaused = false;

    SpinInfo checkSpin();
    void spawnPiece();
    bool collision(int nx, int ny, Piece p);
    void handleInput();
    void finalizeTyping();
    void handleMainMenu(string k);
    void handleScoreboardMenu(string k);
    void handlePlayMenu(string k);
    void handleOptionMenu(string k);
    void handleHandlingMenu(string k);
    void handleVisualMenu(string k);
    void handleControlMenu(string k);
    void handleIngameInput(string k);
    void move(int dx);
    void moveDown();
    bool isOnGround();
    vector<pair<int,int>> getIShape(int state);
    void rotate(int r_mode);
    void hold();
    void resetDecay();
    void hardDrop();
    void lock();
    void update(float dt);
    string getWaveColor();
    string getStr(int type, bool ghost = false, bool isClearing = false, int x = -1, int y = -1);
    void drawOverlay(string title, vector<string> options, string colorCode);
    void drawNameInput();
    void drawKeystrokes();
    string dVal(string v, int i);
    string bkt(string side, int idx);
    void printMenuLine(string label, string value, int index, int width = 26);
    string getRating();
    void draw();
};
