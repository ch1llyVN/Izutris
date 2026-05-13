#pragma once
#include <string>
#include <map>
using namespace std;

struct Config {
    int das = 8, arr = 0, sdf = 40, dcd = 0;
    bool uniformSkin = false;
    string skin = "██", emptySkin = ". ", ghostSkin = "##"; 
    string kLeft = "LEFT", kRight = "RIGHT", kSoft = "DOWN", kHard = " ", kCW = "e", kCCW = "q", k180 = "w", kHold = "f";
    string kRestart = "r", kPause = "ESC", kMenu = "m", kLock = "d"; 
    string keyColor = "96"; 
    bool showGhost = true;
    bool ghostColored = false;
    bool showSpawnPreview = true; 
    string flameColor = "31"; 
    string flashColor = "107";
    string clearColor = "107";
    string lightningColor = "97"; 
    string comboClr = "97"; 
    map<int, string> pieceSkins = {
        {1, "[]"}, {2, "[]"}, {3, "[]"}, {4, "[]"}, {5, "[]"}, {6, "[]"}, {7, "[]"}
    };

    void load(string filename);
    void save(string filename);
};
