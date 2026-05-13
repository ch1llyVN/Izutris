#pragma once
#include <vector>
#include <random>
using namespace std;

struct Piece { 
    vector<pair<int, int>> cells; 
    int type; 
    int rotationState = 0; 
};

class SevenBag {
    vector<int> bag; mt19937 rng;
public:
    SevenBag() : rng(random_device{}()) {}
    void reset() { bag.clear(); }
    Piece getPiece(int type) {
        switch(type) {
            case 1: return {{{0,0}, {-1,0}, {1,0}, {0,-1}}, 1, 0}; 
            case 2: return {{{0,0}, {1,0}, {0,1}, {1,1}}, 2, 0};  
            case 3: return {{{-1,0}, {0,0}, {1,0}, {2,0}}, 3, 0}; 
            case 4: return {{{-1,0}, {0,0}, {0,-1}, {1,-1}}, 4, 0}; 
            case 5: return {{{-1,-1}, {0,-1}, {0,0}, {1,0}}, 5, 0}; 
            case 6: return {{{-1,-1}, {-1,0}, {0,0}, {1,0}}, 6, 0}; 
            case 8: return {{{0,0}}, 8, 0}; 
            default: return {{{-1,0}, {0,0}, {1,0}, {1,-1}}, 7, 0}; 
        }
    }
    int nextType() {
        if (bag.empty()) { bag = {1, 2, 3, 4, 5, 6, 7}; shuffle(bag.begin(), bag.end(), rng); }
        int t = bag.back(); bag.pop_back(); return t;
    }
};
