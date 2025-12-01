#include <iostream>
#include <iomanip>
#include <unistd.h>
#include "MPU6050_Sensor.h"

int main() {

    MPU6050_Sensor imu1("/dev/i2c-1", 0x68);
    
    if (!imu1.init()) {
        std::cerr << "Failed to initialize MPU6050 on /dev/i2c-1" << std::endl;
        return 1;
    }
    
    std::cout << "Sensor: " << imu1.whoAmI() << std::endl;
    
    // Основной цикл измерений
    while (true) {
        const IMU::data& data = imu1.getIMUData();
        
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\033[2J\033[1;1H"; // Очистка экрана (для Linux)
        
        std::cout << "=== MPU6050 IMU Data ===" << std::endl;
        
        // Углы
        std::cout << "Angles:" << std::endl;
        std::cout << "  Roll:  " << std::setw(6) << data.angle.roll << "°" << std::endl;
        std::cout << "  Pitch: " << std::setw(6) << data.angle.pitch << "°" << std::endl;
        std::cout << "  Yaw:   " << std::setw(6) << data.angle.yaw << "°" << std::endl;
        
        // Акселерометр
        std::cout << "Acceleration:" << std::endl;
        std::cout << "  X: " << std::setw(6) << data.acceleration.x << " g" << std::endl;
        std::cout << "  Y: " << std::setw(6) << data.acceleration.y << " g" << std::endl;
        std::cout << "  Z: " << std::setw(6) << data.acceleration.z << " g" << std::endl;
        
        // Гироскоп
        std::cout << "Gyroscope:" << std::endl;
        std::cout << "  X: " << std::setw(6) << data.gyroscope.x << " °/s" << std::endl;
        std::cout << "  Y: " << std::setw(6) << data.gyroscope.y << " °/s" << std::endl;
        std::cout << "  Z: " << std::setw(6) << data.gyroscope.z << " °/s" << std::endl;
        
        usleep(100000); // 100ms обновление
    }
    
    return 0;
}