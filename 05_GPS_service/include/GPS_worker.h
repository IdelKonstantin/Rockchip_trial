#ifndef GPS_WORKER_HPP
#define GPS_WORKER_HPP

#include <cstdint>
#include <string>
#include <cstddef>
#include <vector>

#include "nlohmann.h"

////////////////////////////////////////////////////////////////////////////////////////////

enum class GPS_DIRS {
    NORTH = 0,      // С.Ш
    SOUTH,          // Ю.Ш
    EAST,           // В.Д
    WEST            // З.Д
};

enum class GPS_STATUS {
    INVALID = 0,    // Не валидные данные после парсинга NMEA
    CORRECT         // Статус - ОК, данные валидны
};

struct GPS_loc_t {
    GPS_DIRS direction;    // Полушарие (С/Ю для широты и В/З для долготы)
    uint16_t deg;          // Градусы
    uint16_t min;          // Минуты
    uint16_t sec;          // Секунды дуги
};

struct GPS_time_t {
    uint16_t hour;
    uint16_t min;
    uint16_t sec;
};

struct GPS_date_t {
    uint16_t year;         // Дата
    uint16_t month;
    uint16_t day;
};

struct GPS_angle_t {
    uint16_t deg;
    uint16_t min;
    uint16_t sec;
};

struct GPS_data_t {
    GPS_STATUS SatIsCorrect;   // Флаг что спутники найдены и видимы
    GPS_STATUS LocIsCorrect;   // Флаг корректности местоположения
    GPS_loc_t latitude;        // Широта (с указанием полушария)
    GPS_loc_t longitude;       // Долгота (с указанием полушария)
    GPS_date_t date;           // Дата
    GPS_time_t time;           // время по UTC
    GPS_angle_t heading;       // курс при движении с прибором
    float speed;               // скорость в км/ч
    uint16_t sats;             // Количество активных спутников
};

////////////////////////////////////////////////////////////////////////////////////////////

const uint16_t NMEA_READER_BUFFER_SIZE = 1024;
const uint16_t JSON_GPS_RESULT_SIZE = 1024;

class DataBuffer {

private:

    std::string m_buffer;
    size_t m_maxSize;

public:

    DataBuffer(size_t size = NMEA_READER_BUFFER_SIZE);
    
    void append(const char* data, size_t length);
    std::vector<std::string> extractLines();
    void clear();
    bool empty() const;
    size_t size() const;
};

////////////////////////////////////////////////////////////////////////////////////////////

class GPSFileReader {

private:

    std::string m_fifoPath; // TODO: 
    int m_fd;

public:

    GPSFileReader(const std::string& path);  
    ~GPSFileReader();
    
    bool openFd();
    void closeFd();
    int readData(char* buffer, size_t size);
    bool isOpen() const;
};

////////////////////////////////////////////////////////////////////////////////////////////

class NMEAParser {

private:

    std::vector<std::string> tokenize(const std::string& data);
public:

    bool parseCoordinate(const std::string& str, GPS_loc_t& loc, char hemisphere);
    bool parseTime(const std::string& str, GPS_time_t& time);
    bool parseDate(const std::string& str, GPS_date_t& date);
    bool parseRMC(const std::string& data, GPS_data_t& gps);
    bool parseGGA(const std::string& data, GPS_data_t& gps);
    bool verifyChecksum(const std::string& nmea_str);
    void parseNMEA(const std::string& nmea_str, GPS_data_t& gps_data);
};

////////////////////////////////////////////////////////////////////////////////////////////

class GPSDataPrinter {

public:

    static void print(const GPS_data_t& gps);
};

////////////////////////////////////////////////////////////////////////////////////////////

class GPSSerializer {

private:

    std::string m_resultJson;
    nlohmann::json m_responceJson;

public:

    GPSSerializer();
    const std::string& serializeResult(const GPS_data_t& data);
};

////////////////////////////////////////////////////////////////////////////////////////////

class GPSWorker {

private:

    GPSFileReader file_reader;
    DataBuffer data_buffer;
    NMEAParser nmea_parser;
    GPSSerializer json_serializer;

    GPS_data_t m_GPSdata;

public:

    GPSWorker(const std::string& Fd_path);
    bool initialize();
    void processData();
    void cleanup();
    const std::string& serializeResult(const GPS_data_t& data);
    
    const GPS_data_t& getGPSData() const;

private:

    void processLine(const std::string& line);
};
#endif // GPS_WORKER_HPP