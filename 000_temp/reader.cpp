#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <ctime>
#include <thread>
#include <chrono>
#include <atomic>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iomanip>

const std::string FIFO_PATH = "/tmp/gps_pipe";
const size_t BUFFER_SIZE = 1024;

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

class DataBuffer {
private:
    std::string buffer;
    size_t max_size;

public:
    DataBuffer(size_t size = BUFFER_SIZE) : max_size(size) {}
    
    void append(const char* data, size_t length) {
        for (size_t i = 0; i < length && buffer.length() < max_size; i++) {
            char c = data[i];
            if (c == '\n' || c == '\r') {
                // Конец строки - обрабатываем позже
                buffer += c;
            } else {
                buffer += c;
            }
        }
    }
    
    std::vector<std::string> extractLines() {
        std::vector<std::string> lines;
        std::string line;
        
        for (char c : buffer) {
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
        buffer = line;
        
        return lines;
    }
    
    void clear() {
        buffer.clear();
    }
    
    bool empty() const {
        return buffer.empty();
    }
    
    size_t size() const {
        return buffer.length();
    }
};

class FIFOReader {
private:
    std::string fifo_path;
    int fd;

public:
    FIFOReader(const std::string& path = FIFO_PATH) : fifo_path(path), fd(-1) {
        // Создаем FIFO если не существует
        if (access(fifo_path.c_str(), F_OK) == -1) {
            if (mkfifo(fifo_path.c_str(), 0666) == -1) {
                throw std::runtime_error("Ошибка создания FIFO: " + std::string(strerror(errno)));
            }
            std::cout << "FIFO создан: " << fifo_path << std::endl;
        }
    }
    
    ~FIFOReader() {
        close();
    }
    
    bool open() {
        fd = ::open(fifo_path.c_str(), O_RDONLY);
        return fd != -1;
    }
    
    void close() {
        if (fd != -1) {
            ::close(fd);
            fd = -1;
        }
    }
    
    int readData(char* buffer, size_t size) {
        if (fd == -1) return -1;
        return ::read(fd, buffer, size - 1);
    }
    
    bool isOpen() const {
        return fd != -1;
    }
};

class NMEAParser {
private:
    std::vector<std::string> tokenize(const std::string& data) {
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

public:
    bool parseCoordinate(const std::string& str, GPS_loc_t& loc, char hemisphere) {
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

    bool parseTime(const std::string& str, GPS_time_t& time) {
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

    bool parseDate(const std::string& str, GPS_date_t& date) {
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

    bool parseRMC(const std::string& data, GPS_data_t& gps) {
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

    bool parseGGA(const std::string& data, GPS_data_t& gps) {
        auto tokens = tokenize(data);
        
        if (tokens.size() < 7) return false;
        
        // Парсим количество спутников
        gps.sats = std::stoi(tokens[7]);
        gps.SatIsCorrect = (gps.sats >= 3) ? GPS_STATUS::CORRECT : GPS_STATUS::INVALID;
        
        return true;
    }

    bool verifyChecksum(const std::string& nmea_str) {
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

    void parseNMEA(const std::string& nmea_str, GPS_data_t& gps_data) {
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
};

class GPSDataPrinter {
public:
    static void print(const GPS_data_t& gps) {
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
};

class GPSWorker {
private:
    FIFOReader fifo_reader;
    DataBuffer data_buffer;
    NMEAParser nmea_parser;
    GPSDataPrinter data_printer;
    std::atomic<bool>& keep_running;

public:
    GPSWorker(std::atomic<bool>& running, const std::string& fifo_path = FIFO_PATH) 
        : fifo_reader(fifo_path), keep_running(running) {}
    
    void initialize() {
        std::cout << "GPS Worker инициализирован. Ожидаю NMEA данные..." << std::endl;
        std::cout << "Для выхода нажмите Ctrl+C" << std::endl << std::endl;
    }
    
    void processData() {
        if (!fifo_reader.open()) {
            std::cerr << "Ошибка открытия FIFO: " << strerror(errno) << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return;
        }
        
        char read_buffer[BUFFER_SIZE];
        int bytes_read = fifo_reader.readData(read_buffer, BUFFER_SIZE);
        
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
        
        fifo_reader.close();
    }
    
    void cleanup() {
        fifo_reader.close();
        std::cout << "GPS Worker завершил работу." << std::endl;
    }

private:
    void processLine(const std::string& line) {
        // Проверяем что это NMEA сообщение (начинается с $)
        if (!line.empty() && line[0] == '$') {
            // Парсим NMEA сообщение
            GPS_data_t gps_data;
            nmea_parser.parseNMEA(line, gps_data);
            
            // Выводим результат
            data_printer.print(gps_data);
        }
    }
};

std::atomic<bool> keep_running{true};

void signal_handler(int sig) {
    keep_running = false;
    std::cout << "\nПолучен сигнал завершения. Завершаю работу..." << std::endl;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        GPSWorker worker(keep_running);
        worker.initialize();
        
        while (keep_running) {

            std::cout << "*******" << std::endl;

            worker.processData();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        worker.cleanup();
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return 0;
}