#include <iostream>
#include <string>
#include <cstring>
#include <ctime>
#include <random>
#include <thread>
#include <chrono>
#include <iomanip>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

const std::string FIFO_PATH = "/tmp/gps_pipe";
const size_t BUFFER_SIZE = 256;

class NMEAGenerator {
private:
    std::mt19937 rng;
    std::uniform_int_distribution<int> sat_dist;
    std::uniform_real_distribution<float> speed_dist;

    char calculateChecksum(const std::string& data) {
        char checksum = 0;
        // Пропускаем начальный '$'
        for (size_t i = 1; i < data.length(); i++) {
            if (data[i] == '*') break;
            checksum ^= data[i];
        }
        return checksum;
    }

public:
    NMEAGenerator() : 
        rng(std::random_device{}()),
        sat_dist(6, 9),
        speed_dist(5.0f, 15.0f) 
    {}

    std::string generateRMC(double lat, double lon, float speed, float course) {
        std::time_t now = std::time(nullptr);
        std::tm* utc = std::gmtime(&now);
        
        // Форматируем время
        std::ostringstream time_ss;
        time_ss << std::setfill('0') << std::setw(2) << utc->tm_hour
                << std::setw(2) << utc->tm_min << std::setw(2) << utc->tm_sec << ".000";
        
        // Форматируем дату
        std::ostringstream date_ss;
        date_ss << std::setw(2) << utc->tm_mday << std::setw(2) << (utc->tm_mon + 1)
                << std::setw(2) << (utc->tm_year % 100);
        
        // Форматируем координаты
        int lat_deg = static_cast<int>(lat);
        double lat_min = (lat - lat_deg) * 60;
        int lon_deg = static_cast<int>(lon);
        double lon_min = (lon - lon_deg) * 60;
        
        std::ostringstream lat_ss, lon_ss;
        lat_ss << std::setfill('0') << std::setw(2) << lat_deg 
               << std::fixed << std::setprecision(3) << std::setw(6) << lat_min;
        lon_ss << std::setfill('0') << std::setw(3) << lon_deg 
               << std::fixed << std::setprecision(3) << std::setw(6) << lon_min;
        
        // Генерируем RMC сообщение
        std::ostringstream rmc_ss;
        rmc_ss << "$GPRMC," << time_ss.str() << ",A,"
               << lat_ss.str() << ",N," << lon_ss.str() << ",E,"
               << std::fixed << std::setprecision(1) << speed << ","
               << std::setprecision(1) << course << ","
               << date_ss.str() << ",0.0,E,A*";
        
        std::string rmc = rmc_ss.str();
        
        // Добавляем checksum
        char checksum = calculateChecksum(rmc);
        std::ostringstream checksum_ss;
        checksum_ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') 
                    << static_cast<unsigned int>(static_cast<unsigned char>(checksum));
        rmc += checksum_ss.str();
        
        return rmc;
    }

    std::string generateGGA(double lat, double lon, int satellites) {
        std::time_t now = std::time(nullptr);
        std::tm* utc = std::gmtime(&now);
        
        // Форматируем время
        std::ostringstream time_ss;
        time_ss << std::setfill('0') << std::setw(2) << utc->tm_hour
                << std::setw(2) << utc->tm_min << std::setw(2) << utc->tm_sec << ".000";
        
        // Форматируем координаты
        int lat_deg = static_cast<int>(lat);
        double lat_min = (lat - lat_deg) * 60;
        int lon_deg = static_cast<int>(lon);
        double lon_min = (lon - lon_deg) * 60;
        
        std::ostringstream lat_ss, lon_ss;
        lat_ss << std::setfill('0') << std::setw(2) << lat_deg 
               << std::fixed << std::setprecision(3) << std::setw(6) << lat_min;
        lon_ss << std::setfill('0') << std::setw(3) << lon_deg 
               << std::fixed << std::setprecision(3) << std::setw(6) << lon_min;
        
        // Генерируем GGA сообщение
        std::ostringstream gga_ss;
        gga_ss << "$GPGGA," << time_ss.str() << ","
               << lat_ss.str() << ",N," << lon_ss.str() << ",E,1,"
               << std::setw(2) << std::setfill('0') << satellites << ",1.0,100.0,M,0.0,M,,*";
        
        std::string gga = gga_ss.str();
        
        // Добавляем checksum
        char checksum = calculateChecksum(gga);
        std::ostringstream checksum_ss;
        checksum_ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') 
                    << static_cast<unsigned int>(static_cast<unsigned char>(checksum));
        gga += checksum_ss.str();
        
        return gga;
    }

    // Публичные методы для получения случайных значений
    int getRandomSatellites() {
        return sat_dist(rng);
    }

    float getRandomSpeed() {
        return speed_dist(rng);
    }
};

int main() {
    // Создаем FIFO если не существует
    if (access(FIFO_PATH.c_str(), F_OK) == -1) {
        if (mkfifo(FIFO_PATH.c_str(), 0666) == -1) {
            std::cerr << "Ошибка создания FIFO: " << strerror(errno) << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "FIFO создан: " << FIFO_PATH << std::endl;
    }
    
    std::cout << "GPS Writer запущен. Генерация NMEA данных..." << std::endl;
    std::cout << "Для выхода нажмите Ctrl+C" << std::endl << std::endl;
    
    NMEAGenerator generator;
    
    // Начальные координаты (пример: Москва)
    double latitude = 55.7558;
    double longitude = 37.6173;
    float speed = 0.0;
    int course = 0;
    
    while (true) {
        std::cout << "Открываю FIFO для записи..." << std::endl;
        int fd = open(FIFO_PATH.c_str(), O_WRONLY);
        if (fd == -1) {
            std::cerr << "Ошибка открытия FIFO: " << strerror(errno) << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        // Генерируем сообщения используя публичные методы
        std::string rmc = generator.generateRMC(latitude, longitude, speed, static_cast<float>(course));
        std::string gga = generator.generateGGA(latitude, longitude, generator.getRandomSatellites());
        
        // Формируем полное сообщение
        std::string full_message = rmc + "\r\n" + gga + "\r\n";
        
        int bytes_written = write(fd, full_message.c_str(), full_message.length());
        if (bytes_written == -1) {
            std::cerr << "Ошибка записи: " << strerror(errno) << std::endl;
        } else {
            std::cout << "Отправлено " << bytes_written << " байт" << std::endl;
            std::cout << "RMC: " << rmc << std::endl;
            std::cout << "GGA: " << gga << std::endl;
        }
        
        close(fd);
        
        // Обновляем данные для следующей итерации
        latitude += 0.0001;  // Небольшое перемещение
        longitude += 0.0001;
        speed = generator.getRandomSpeed();  // Используем публичный метод
        course = (course + 5) % 360;
        
        std::cout << "--- Следующие данные через 2 секунды ---" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    return 0;
}