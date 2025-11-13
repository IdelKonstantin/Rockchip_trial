#ifndef RANDOM_UUID_GENERATOR_H
#define RANDOM_UUID_GENERATOR_H

#include <string>

class RandomUUIDGenerator {
    
public:
    static std::string generateToken();
    static std::string generateHexToken();
};

#endif // RANDOM_UUID_GENERATOR_H