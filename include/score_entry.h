#pragma once
#include <string>
using namespace std;

struct ScoreEntry {
    string name;
    string mode;
    string detail; 
    string result; 

    string toString() const {
        char buf[128];
        snprintf(buf, sizeof(buf), "%-10s | %-8s | %-4s | %s", name.c_str(), mode.c_str(), detail.c_str(), result.c_str());
        return string(buf);
    }
};
