#include "BH1750_Sensor.h"
#include <stdexcept>
#include <cmath>

// Конструктор
BH1750_Sensor::BH1750_Sensor(const std::string& i2c_device, uint8_t address) 
    : i2c_fd(-1), i2c_addr(address), initialized(false), i2c_device(i2c_device) {
    
    // Инициализация структуры данных
    current_data = {0.0f, light::level::TOTAL_DARKNESS};
}

// Деструктор
BH1750_Sensor::~BH1750_Sensor() {
    if (i2c_fd >= 0) {
        // Выключение датчика
        writeCommand(BH1750_POWER_DOWN);
        close(i2c_fd);
    }
}

// Запись команды в BH1750
bool BH1750_Sensor::writeCommand(uint8_t command) {
    if (i2c_fd < 0) {
        return false;
    }

    if (write(i2c_fd, &command, 1) != 1) {
        return false;
    }
    
    // Небольшая задержка для обработки команды
    usleep(10000);
    return true;
}

// Чтение данных из BH1750
bool BH1750_Sensor::readData(uint16_t &data) {
    if (i2c_fd < 0) {
        return false;
    }

    uint8_t buffer[2];
    if (read(i2c_fd, buffer, 2) != 2) {
        return false;
    }

    data = (buffer[0] << 8) | buffer[1];
    return true;
}

// Определение уровня освещенности на основе люксов
light::level BH1750_Sensor::calculateLightLevel(light::lux_t lux) {
    if (lux < THRESHOLD_TOTAL_DARKNESS) {
        return light::level::TOTAL_DARKNESS;
    } else if (lux < THRESHOLD_VERY_DARK) {
        return light::level::VERY_DARK;
    } else if (lux < THRESHOLD_DARK) {
        return light::level::DARK;
    } else if (lux < THRESHOLD_TWILIGHT) {
        return light::level::TWILIGHT;
    } else if (lux < THRESHOLD_DIM_LIGHT) {
        return light::level::DIM_LIGHT;
    } else if (lux < THRESHOLD_NORMAL_LIGHT) {
        return light::level::NORMAL_LIGHT;
    } else if (lux < THRESHOLD_BRIGHT_LIGHT) {
        return light::level::BRIGHT_LIGHT;
    } else {
        return light::level::VERY_BRIGHT;
    }
}

// Инициализация датчика
bool BH1750_Sensor::init() {
    if (initialized) {
        return true;
    }

    // Открываем I2C устройство
    i2c_fd = open(i2c_device.c_str(), O_RDWR);
    if (i2c_fd < 0) {
        return false;
    }
    
    // Устанавливаем адрес устройства
    if (ioctl(i2c_fd, I2C_SLAVE, i2c_addr) < 0) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Инициализация BH1750
    if (!writeCommand(BH1750_POWER_ON)) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    if (!writeCommand(BH1750_CONTINUOUS_HIGH_RES_MODE)) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Небольшая задержка для стабилизации
    usleep(100000);

    // Тестовое чтение для проверки работоспособности
    uint16_t test_data;
    if (!readData(test_data)) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    initialized = true;
    return true;
}

// Получение данных об освещенности
const light::data& BH1750_Sensor::getLightData() {
    if (!initialized) {
        return current_data;
    }

    uint16_t raw_data;
    if (readData(raw_data)) {
        // Конвертация сырых данных в люксы
        // Формула: lux = raw_data / 1.2 * (69 / X) 
        // где X - время измерения в мс (обычно 69ms для высокого разрешения)
        current_data.lightIntencity = static_cast<light::lux_t>(raw_data) / 1.2f;
        current_data.lightLevel = calculateLightLevel(current_data.lightIntencity);
    }

    return current_data;
}

// Идентификация датчика
const std::string BH1750_Sensor::whoAmI() const {
    return "BH1750";
}