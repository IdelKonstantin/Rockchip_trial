#include "random_UUID_generator.h"
#include <random>
#include <algorithm>

std::string RandomUUIDGenerator::generateToken() {
    
    const std::string chars =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, chars.size() - 1);
    
    std::string token;
    token.reserve(16);
    
    for (int i = 0; i < 16; ++i) {
        token += chars[distribution(generator)];
    }
    
    return token;
}

std::string RandomUUIDGenerator::generateHexToken() {
    const std::string hex_chars = "0123456789abcdef";
    
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, 15);
    
    std::string token;
    token.reserve(16);
    
    for (int i = 0; i < 16; ++i) {
        token += hex_chars[distribution(generator)];
    }
    
    return token;
}