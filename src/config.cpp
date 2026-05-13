#include "config.h"
#include <fstream>

void Config::load(string filename) {
    ifstream file(filename);
    if (!file.is_open()) return;
    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t sep = line.find('=');
        if (sep == string::npos) continue;
        string key = line.substr(0, sep);
        string val = line.substr(sep + 1);
        if (key == "DAS") das = stoi(val);
        else if (key == "ARR") arr = stoi(val);
        else if (key == "SDF") sdf = stoi(val);
        else if (key == "DCD") dcd = stoi(val);
        else if (key == "UNIFORM") uniformSkin = (val == "ON");
        else if (key == "SKIN") skin = val;
        else if (key == "EMPTY") emptySkin = val;
        else if (key == "GHOST_SKIN") ghostSkin = val;
        else if (key == "GHOST") showGhost = (val == "ON");
        else if (key == "GHOST_CLR") ghostColored = (val == "ON");
        else if (key == "SPAWN_PREV") showSpawnPreview = (val == "ON"); 
        else if (key == "FLAME_CLR") flameColor = val;
        else if (key == "CLEAR_CLR") clearColor = val;
        else if (key == "FLASH_CLR") flashColor = val;
        else if (key == "KEY_CLR") keyColor = val;
        else if (key == "LIGHT_CLR") lightningColor = val;
        else if (key == "COMBO_CLR") comboClr = val;
        else if (key == "LEFT") kLeft = val;
        else if (key == "RIGHT") kRight = val;
        else if (key == "SOFT") kSoft = val;
        else if (key == "CW") kCW = val;
        else if (key == "CCW") kCCW = val;
        else if (key == "R180") k180 = val;
        else if (key == "HOLD") kHold = val;
        else if (key == "RESTART") kRestart = val;
        else if (key == "PAUSE") kPause = val;
        else if (key == "MENU") kMenu = val;
        else if (key == "LOCK") kLock = val;
    }
}

void Config::save(string filename) {
    ofstream file(filename);
    file << "DAS=" << das << "\n";
    file << "ARR=" << arr << "\n";
    file << "SDF=" << sdf << "\n";
    file << "DCD=" << dcd << "\n";
    file << "UNIFORM=" << (uniformSkin ? "ON" : "OFF") << "\n";
    file << "SKIN=" << skin << "\n";
    file << "EMPTY=" << emptySkin << "\n";
    file << "GHOST_SKIN=" << ghostSkin << "\n";
    file << "GHOST=" << (showGhost ? "ON" : "OFF") << "\n";
    file << "GHOST_CLR=" << (ghostColored ? "ON" : "OFF") << "\n";
    file << "SPAWN_PREV=" << (showSpawnPreview ? "ON" : "OFF") << "\n"; 
    file << "FLAME_CLR=" << flameColor << "\n";
    file << "CLEAR_CLR=" << clearColor << "\n";
    file << "FLASH_CLR=" << flashColor << "\n";
    file << "KEY_CLR=" << keyColor << "\n";
    file << "LIGHT_CLR=" << lightningColor << "\n";
    file << "COMBO_CLR=" << comboClr << "\n";
    file << "LEFT=" << kLeft << "\n";
    file << "RIGHT=" << kRight << "\n";
    file << "SOFT=" << kSoft << "\n";
    file << "CW=" << kCW << "\n";
    file << "CCW=" << kCCW << "\n";
    file << "R180=" << k180 << "\n";
    file << "HOLD=" << kHold << "\n";
    file << "RESTART=" << kRestart << "\n";
    file << "PAUSE=" << kPause << "\n";
    file << "MENU=" << kMenu << "\n";
    file << "LOCK=" << kLock << "\n";
}
