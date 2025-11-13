#include "CUUIDGenerator.h"

const std::string& uuid::CUUIDGenerator::getTemplateString() {

    const static std::string sAllowed{"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"};
    return sAllowed;
}

std::string uuid::CUUIDGenerator::getNewUUID(uint64_t uuidType, size_t nRequiredLen) {
    std::string sUUID;
    
    if (uuidType & generator_type::SYSTEM_GENERATOR) {

        sUUID = getRandomSystemString();
    }

    if (uuidType & generator_type::RANDOM_GENERATOR) {

        sUUID += getRandomString(nRequiredLen);
    }

    if (uuidType & generator_type::TIME_GENERATOR) {

        sUUID += getCurrentTimeString();
    }

    if (nRequiredLen > DEFAULT_UUID_STRING_LEN)
    {
        if (nRequiredLen != sUUID.length())
        {
            if (nRequiredLen > sUUID.length())
            {
                std::string sTail = getRandomString(nRequiredLen - sUUID.length());
                sUUID += sTail;
            } else {
                std::random_shuffle(sUUID.begin(), sUUID.end());
                sUUID = sUUID.substr(0, nRequiredLen);
            }
        }
    }
    else
        sUUID = getMD5(sUUID);
    return sUUID;
}

std::string uuid::CUUIDGenerator::getMD5(const std::string &strSource) {

    std::array<unsigned char, MD5_DIGEST_LENGTH> sResultMD5;
    MD5(reinterpret_cast<const unsigned char *>(strSource.data()), strSource.size(), sResultMD5.data());

    std::string sResult;
    sResult.reserve(2 * MD5_DIGEST_LENGTH);
    for (unsigned char hashChar: sResultMD5)
    {
        sResult += "0123456789ABCDEF"[hashChar / 16];
        sResult += "0123456789ABCDEF"[hashChar % 16];
    }

    return sResult;
}

std::string uuid::CUUIDGenerator::getRandomSystemString() {

    uuid_t uuidObj;
    uuid_generate(uuidObj);

    std::array<char, SIZE_OF_SYSTEM_UUID> arSystemUUID;
    uuid_unparse(uuidObj, &arSystemUUID.front());

    std::string systemUUID(std::begin(arSystemUUID), std::end(arSystemUUID));
    systemUUID.erase(std::remove(systemUUID.begin(), systemUUID.end(),L'-'), systemUUID.end());
    return systemUUID;
}

std::string uuid::CUUIDGenerator::getCurrentTimeString() {

    time_stamp_microseconds ts = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
    std::string sCurTime = std::to_string(ts.time_since_epoch().count());
    return sCurTime;
}

std::string uuid::CUUIDGenerator::getRandomString(size_t nLen) {

    thread_local static std::mt19937 rndEng{std::random_device{}()};
    thread_local static std::uniform_int_distribution<std::string::size_type> rndIndex(0, uuid::CUUIDGenerator::getTemplateString().length() - 1);

    std::string sRandom;

    sRandom.reserve(nLen);

    while(nLen--)
        sRandom += getTemplateString()[rndIndex(rndEng)];

    return sRandom;
}


std::string uuid::CUUIDGenerator::getRandomStringFast(size_t nLen) {
    
    std::string sRandom;
    sRandom.reserve(nLen);

    while(nLen--)
        sRandom += getTemplateString()[uuid::CUUIDGenerator::getBoundedRandValue(getTemplateString().length() - 1)];

    return sRandom;
}

// Экспериментальная быстрая функция генерации случайных чисел
// в заданном диапазоне
// см. http://www.pcg-random.org/posts/bounded-rands.html
uint32_t uuid::CUUIDGenerator::getBoundedRandValue(uint32_t range) {
    
    thread_local static std::mt19937 rng{std::random_device{}()};
    uint32_t x = rng();
    uint64_t m = uint64_t(x) * uint64_t(range);
    uint32_t l = uint32_t(m);
    if (l < range) {
        uint32_t t = -range;
        if (t >= range) {
            t -= range;
            if (t >= range)
                t %= range;
        }
        while (l < t) {
            x = rng();
            m = uint64_t(x) * uint64_t(range);
            l = uint32_t(m);
        }
    }
    return m >> 32;
}