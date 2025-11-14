#include "GPS_worker.h"
#include "CFastLog.h"

////////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cstring>
#include <iomanip>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

////////////////////////////////////////////////////////////////////////////////////////////

DataBuffer::DataBuffer(size_t size) : m_maxSize(size) {

    m_buffer.reserve(NMEA_READER_BUFFER_SIZE);
}

void DataBuffer::append(const char* data, size_t length) {
    for (size_t i = 0; i < length && m_buffer.length() < m_maxSize; i++) {
        char c = data[i];
        if (c == '\n' || c == '\r') {
            // Конец строки - обрабатываем позже
            m_buffer += c;
        } else {
            m_buffer += c;
        }
    }
}

std::vector<std::string> DataBuffer::extractLines() {
    std::vector<std::string> lines;
    std::string line;
    
    for (char c : m_buffer) {
        if (c == '\n' || c == '\r') {
            if (!line.empty()) {
                lines.push_back(line);
                line.clear();
            }
        } else {
            line += c;
        }
    }
    
    // Сохраняем оставшиеся данные (неполная строка)
    m_buffer = line;  
    return lines;
}

void DataBuffer::clear() {
    m_buffer.clear();
}

bool DataBuffer::empty() const {
    return m_buffer.empty();
}

size_t DataBuffer::size() const {
    return m_buffer.length();
}

////////////////////////////////////////////////////////////////////////////////////////////

GPSFileReader::GPSFileReader(const std::string& path) : m_fifoPath(path), m_fd(-1) {                    /* TODO: переписать для работы с файловым дескриптором устройства а не фифо*/
    // Создаем FIFO если не существует
    if (access(m_fifoPath.c_str(), F_OK) == -1) {
        if (mkfifo(m_fifoPath.c_str(), 0666) == -1) {
            throw std::runtime_error("Ошибка создания FIFO: " + std::string(strerror(errno)));
        }
        std::cout << "FIFO создан: " << m_fifoPath << std::endl;
    }
}

GPSFileReader::~GPSFileReader() {
    closeFd();
}

bool GPSFileReader::openFd() {
    m_fd = open(m_fifoPath.c_str(), O_RDONLY);
    return m_fd != -1;
}

void GPSFileReader::closeFd() {
    if (m_fd != -1) {
        close(m_fd);
        m_fd = -1;
    }
}

int GPSFileReader::readData(char* buffer, size_t size) {
    if (m_fd == -1) return -1;
    return ::read(m_fd, buffer, size - 1);
}

bool GPSFileReader::isOpen() const {
    return m_fd != -1;
}

////////////////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> NMEAParser::tokenize(const std::string& data) {
    std::vector<std::string> tokens;
    std::string token;
    
    for (char c : data) {
        if (c == ',' || c == '*') {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            if (c == '*') break;
        } else {
            token += c;
        }
    }
    
    if (!token.empty()) {
        tokens.push_back(token);
    }
    
    return tokens;
}


bool NMEAParser::parseCoordinate(const std::string& str, GPS_loc_t& loc, char hemisphere) {
    if (str.length() < 7) return false;
    
    try {
        // Извлекаем градусы
        loc.deg = std::stoi(str.substr(0, 2));
        
        // Извлекаем минуты и секунды
        double minutes = std::stod(str.substr(2));
        loc.min = static_cast<uint16_t>(minutes);
        loc.sec = static_cast<uint16_t>((minutes - loc.min) * 60);
        
        // Устанавливаем направление
        switch (hemisphere) {
            case 'N': loc.direction = GPS_DIRS::NORTH; break;
            case 'S': loc.direction = GPS_DIRS::SOUTH; break;
            case 'E': loc.direction = GPS_DIRS::EAST; break;
            case 'W': loc.direction = GPS_DIRS::WEST; break;
            default: return false;
        }
    } catch (const std::exception& e) {
        return false;
    }
    
    return true;
}

bool NMEAParser::parseTime(const std::string& str, GPS_time_t& time) {
    if (str.length() < 6) return false;
    
    try {
        time.hour = std::stoi(str.substr(0, 2));
        time.min = std::stoi(str.substr(2, 2));
        time.sec = std::stoi(str.substr(4, 2));
    } catch (const std::exception& e) {
        return false;
    }
    
    return true;
}

bool NMEAParser::parseDate(const std::string& str, GPS_date_t& date) {
    if (str.length() != 6) return false;
    
    try {
        date.day = std::stoi(str.substr(0, 2));
        date.month = std::stoi(str.substr(2, 2));
        date.year = 2000 + std::stoi(str.substr(4, 2)); // Предполагаем 21 век
    } catch (const std::exception& e) {
        return false;
    }
    
    return true;
}

bool NMEAParser::parseRMC(const std::string& data, GPS_data_t& gps) {
    auto tokens = tokenize(data);
    
    if (tokens.size() < 12) return false;
    if (tokens[2].empty() || tokens[2][0] != 'A') {
        gps.LocIsCorrect = GPS_STATUS::INVALID;
        return false;
    }
    
    gps.LocIsCorrect = GPS_STATUS::CORRECT;
    
    // Парсим время
    parseTime(tokens[1], gps.time);
    
    // Парсим широту и долготу
    parseCoordinate(tokens[3], gps.latitude, tokens[4].empty() ? 'N' : tokens[4][0]);
    parseCoordinate(tokens[5], gps.longitude, tokens[6].empty() ? 'E' : tokens[6][0]);
    
    // Парсим скорость (в узлах, конвертируем в км/ч)
    gps.speed = std::stof(tokens[7]) * 1.852f;
    
    // Парсим курс
    if (!tokens[8].empty()) {
        double course = std::stod(tokens[8]);
        gps.heading.deg = static_cast<uint16_t>(course);
        gps.heading.min = static_cast<uint16_t>((course - gps.heading.deg) * 60);
        gps.heading.sec = static_cast<uint16_t>(((course - gps.heading.deg) * 60 - gps.heading.min) * 60);
    }
    
    // Парсим дату
    parseDate(tokens[9], gps.date);
    
    return true;
}

bool NMEAParser::parseGGA(const std::string& data, GPS_data_t& gps) {
    auto tokens = tokenize(data);
    
    if (tokens.size() < 7) return false;
    
    // Парсим количество спутников
    gps.sats = std::stoi(tokens[7]);
    gps.SatIsCorrect = (gps.sats >= 3) ? GPS_STATUS::CORRECT : GPS_STATUS::INVALID;
    
    return true;
}

bool NMEAParser::verifyChecksum(const std::string& nmea_str) {
    size_t asterisk_pos = nmea_str.find('*');
    if (asterisk_pos == std::string::npos || asterisk_pos < 7) return false;
    
    // Вычисляем ожидаемый checksum
    char calculated_checksum = 0;
    for (size_t i = 1; i < asterisk_pos; ++i) {
        calculated_checksum ^= nmea_str[i];
    }
    
    // Получаем checksum из строки
    std::string received_checksum_str = nmea_str.substr(asterisk_pos + 1, 2);
    unsigned long received_checksum = std::stoul(received_checksum_str, nullptr, 16);
    
    return (calculated_checksum == static_cast<char>(received_checksum));
}

void NMEAParser::parseNMEA(const std::string& nmea_str, GPS_data_t& gps_data) {
    // Инициализируем структуру
    gps_data = GPS_data_t{};
    gps_data.LocIsCorrect = GPS_STATUS::INVALID;
    gps_data.SatIsCorrect = GPS_STATUS::INVALID;
    
    // Проверяем checksum
    if (!verifyChecksum(nmea_str)) {
        std::cout << "Ошибка checksum для: " << nmea_str << std::endl;
        return;
    }
    
    // Парсим в зависимости от типа сообщения
    if (nmea_str.find("$GPRMC") != std::string::npos) {
        parseRMC(nmea_str, gps_data);
    } else if (nmea_str.find("$GPGGA") != std::string::npos) {
        parseGGA(nmea_str, gps_data);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////

void GPSDataPrinter::print(const GPS_data_t& gps) {
    std::cout << "\n=== GPS DATA ===" << std::endl;
    std::cout << "Status: " << (gps.LocIsCorrect == GPS_STATUS::CORRECT ? "VALID" : "INVALID") << std::endl;
    std::cout << "Satellites: " << gps.sats << " (" 
              << (gps.SatIsCorrect == GPS_STATUS::CORRECT ? "VALID" : "INVALID") << ")" << std::endl;
    
    if (gps.LocIsCorrect == GPS_STATUS::CORRECT) {
        std::cout << "Time: " << std::setfill('0') << std::setw(2) << gps.time.hour << ":"
                  << std::setw(2) << gps.time.min << ":" << std::setw(2) << gps.time.sec << " UTC" << std::endl;
        std::cout << "Date: " << std::setw(2) << gps.date.day << "." 
                  << std::setw(2) << gps.date.month << "." << gps.date.year << std::endl;
        
        const char* lat_dir = (gps.latitude.direction == GPS_DIRS::NORTH) ? "N" : "S";
        const char* lon_dir = (gps.longitude.direction == GPS_DIRS::EAST) ? "E" : "W";
        
        std::cout << "Latitude: " << std::setw(2) << gps.latitude.deg << "°" 
                  << std::setw(2) << gps.latitude.min << "'" << std::setw(2) 
                  << gps.latitude.sec << "\" " << lat_dir << std::endl;
        std::cout << "Longitude: " << std::setw(3) << gps.longitude.deg << "°" 
                  << std::setw(2) << gps.longitude.min << "'" << std::setw(2) 
                  << gps.longitude.sec << "\" " << lon_dir << std::endl;
        std::cout << "Speed: " << std::fixed << std::setprecision(1) << gps.speed << " km/h" << std::endl;
        std::cout << "Heading: " << gps.heading.deg << "°" << std::setw(2) 
                  << gps.heading.min << "'" << std::setw(2) << gps.heading.sec << "\"" << std::endl;
    }
    std::cout << "================" << std::endl << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////

GPSWorker::GPSWorker(const std::string& Fd_path) : file_reader(Fd_path) {}

void GPSWorker::processData() {
    
    if (!file_reader.openFd()) {
        std::cerr << "Ошибка открытия FIFO: " << strerror(errno) << std::endl;
        return;
    }
    
    char read_buffer[NMEA_READER_BUFFER_SIZE];
    int bytes_read = file_reader.readData(read_buffer, NMEA_READER_BUFFER_SIZE);
    
    if (bytes_read > 0) {
        read_buffer[bytes_read] = '\0';
        
        // Добавляем данные в буфер
        data_buffer.append(read_buffer, bytes_read);
        
        // Извлекаем и обрабатываем полные строки
        auto lines = data_buffer.extractLines();
        for (const auto& line : lines) {
            processLine(line);
        }
    }
    
    file_reader.closeFd();
}

void GPSWorker::cleanup() {
    file_reader.closeFd();
    std::cout << "GPS Worker завершил работу." << std::endl;
}

void GPSWorker::processLine(const std::string& line) {
    // Проверяем что это NMEA сообщение (начинается с $)
    if (!line.empty() && line[0] == '$') {
        // Парсим NMEA сообщение
        GPS_data_t gps_data;
        nmea_parser.parseNMEA(line, gps_data);
    }
}