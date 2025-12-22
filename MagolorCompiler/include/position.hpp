#pragma once
#include <string>

struct Position {
    int line = 0;
    int character = 0;
    
    bool operator<(const Position& other) const {
        if (line != other.line) return line < other.line;
        return character < other.character;
    }
    
    bool operator<=(const Position& other) const {
        return *this < other || (line == other.line && character == other.character);
    }
};

struct Range {
    Position start;
    Position end;
    
    bool contains(const Position& pos) const {
        return start <= pos && pos <= end;
    }
};

struct Location {
    std::string uri;
    Range range;
};
