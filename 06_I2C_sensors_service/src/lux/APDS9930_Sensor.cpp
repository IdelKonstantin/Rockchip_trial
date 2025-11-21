#include "APDS9930_Sensor.h"
#include <stdexcept>
#include <cmath>

// Конструктор
APDS9930_Sensor::APDS9930_Sensor(const std::string& i2c_device, uint8_t address) 
    : i2c_fd(-1), i2c_addr(address), initialized(false), 
      i2c_device(i2c_device), ch0_calibration(1.0f), ch1_calibration(1.0f) {
    
    current_data = {0.0f, light::level::TOTAL_DARKNESS};
}

// Деструктор
APDS9930_Sensor::~APDS9930_Sensor() {
    if (i2c_fd >= 0) {
        // Выключение всех функций датчика
        writeByte(APDS9930_ENABLE, 0x00);
        close(i2c_fd);
    }
}

// Запись байта в регистр
bool APDS9930_Sensor::writeByte(uint8_t reg, uint8_t value) {
    if (i2c_fd < 0) {
        return false;
    }

    uint8_t data[2] = {reg, value};
    if (write(i2c_fd, data, 2) != 2) {
        return false;
    }
    
    usleep(10000);
    return true;
}

// Чтение байта из регистра
uint8_t APDS9930_Sensor::readByte(uint8_t reg) {
    if (i2c_fd < 0) {
        return 0;
    }

    if (write(i2c_fd, &reg, 1) != 1) {
        return 0;
    }

    uint8_t value;
    if (read(i2c_fd, &value, 1) != 1) {
        return 0;
    }

    return value;
}

// Чтение слова (2 байта) из регистра
uint16_t APDS9930_Sensor::readWord(uint8_t reg_low) {
    if (i2c_fd < 0) {
        return 0;
    }

    uint8_t data[2];
    
    // Чтение младшего байта
    if (write(i2c_fd, &reg_low, 1) != 1) {
        return 0;
    }
    if (read(i2c_fd, &data[0], 1) != 1) {
        return 0;
    }

    // Чтение старшего байта
    uint8_t reg_high = reg_low + 1;
    if (write(i2c_fd, &reg_high, 1) != 1) {
        return 0;
    }
    if (read(i2c_fd, &data[1], 1) != 1) {
        return 0;
    }

    return (data[1] << 8) | data[0];
}

// Включение/выключение режимов работы
bool APDS9930_Sensor::setMode(uint8_t mode, bool enable) {
    uint8_t reg_val = readByte(APDS9930_ENABLE);
    
    if (enable) {
        reg_val |= mode;
    } else {
        reg_val &= ~mode;
    }
    
    return writeByte(APDS9930_ENABLE, reg_val);
}

// Настройка датчика освещенности
bool APDS9930_Sensor::setupLightSensor() {
    // Установка времени интеграции ALS (2.72ms)
    if (!writeByte(APDS9930_ATIME, APDS9930_DEFAULT_ATIME)) {
        return false;
    }

    // Установка времени ожидания
    if (!writeByte(APDS9930_WTIME, APDS9930_DEFAULT_WTIME)) {
        return false;
    }

    // Настройка persistence
    if (!writeByte(APDS9930_PERS, APDS9930_DEFAULT_PERS)) {
        return false;
    }

    // Настройка управления (100mA LED drive, 1x proximity gain, 1x ALS gain)
    if (!writeByte(APDS9930_CONTROL, 0x20)) {
        return false;
    }

    return true;
}

// Расчет освещенности в люксах
light::lux_t APDS9930_Sensor::calculateLux(uint16_t ch0, uint16_t ch1) {
    if (ch0 == 0) {
        return 0.0f;
    }

    // Расчет соотношения IR/Visible
    double ratio = static_cast<double>(ch1) / static_cast<double>(ch0);
    
    // Эмпирические коэффициенты для APDS-9930 (из даташита)
    double lux = 0.0;
    
    if (ratio < 0.5) {
        lux = (0.0304 * ch0) - (0.062 * ch0 * pow(ratio, 1.4));
    } else if (ratio < 0.61) {
        lux = (0.0224 * ch0) - (0.031 * ch1);
    } else if (ratio < 0.80) {
        lux = (0.0128 * ch0) - (0.0153 * ch1);
    } else if (ratio < 1.30) {
        lux = (0.00146 * ch0) - (0.00112 * ch1);
    } else {
        lux = 0;
    }

    // Применение калибровочных коэффициентов
    lux = lux * ch0_calibration;

    return static_cast<light::lux_t>(lux > 0 ? lux : 0);
}

// Определение уровня освещенности на основе люксов
light::level APDS9930_Sensor::calculateLightLevel(light::lux_t lux) {
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
bool APDS9930_Sensor::init() {
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

    // Проверяем ID датчика
    uint8_t id = readByte(APDS9930_ID);
    if (id != APDS9930_ID_VALUE) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Выключаем все функции для чистой настройки
    if (!writeByte(APDS9930_ENABLE, 0x00)) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Настройка датчика освещенности
    if (!setupLightSensor()) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Включение питания
    if (!setMode(APDS9930_PON, true)) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    usleep(10000); // 10ms задержка

    // Включение датчика освещенности
    if (!setMode(APDS9930_AEN, true)) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Задержка для первого измерения
    usleep(30000); // 30ms

    // Тестовое чтение для проверки работоспособности
    uint16_t test_ch0 = readWord(APDS9930_CDATAL);
    uint16_t test_ch1 = readWord(APDS9930_IRDATAL);
    
    if (test_ch0 == 0 && test_ch1 == 0) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Калибровка на основе тестовых значений
    if (test_ch0 > 0) {
        ch0_calibration = 1.0f; // Можно настроить при необходимости
    }

    initialized = true;
    return true;
}

// Получение данных об освещенности
const light::data& APDS9930_Sensor::getLightData() {
    if (!initialized) {
        return current_data;
    }

    // Проверка готовности данных
    uint8_t status = readByte(APDS9930_STATUS);
    if (!(status & 0x01)) { // Проверка бита ALS valid
        return current_data;
    }

    // Чтение данных с двух каналов
    uint16_t ch0 = readWord(APDS9930_CDATAL); // Видимый + ИК
    uint16_t ch1 = readWord(APDS9930_IRDATAL); // Только ИК

    if (ch0 != 0 || ch1 != 0) {
        current_data.lightIntencity = calculateLux(ch0, ch1);
        current_data.lightLevel = calculateLightLevel(current_data.lightIntencity);
    }

    return current_data;
}

// Идентификация датчика
const std::string APDS9930_Sensor::whoAmI() const {
    return "APDS-9930";
}

// Получение данных приближения
uint16_t APDS9930_Sensor::getProximity() {
    if (!initialized) {
        return 0;
    }

    // Проверка готовности данных приближения
    uint8_t status = readByte(APDS9930_STATUS);
    if (!(status & 0x02)) { // Проверка бита proximity valid
        return 0;
    }

    return readWord(APDS9930_PDATAL);
}

// Включение/выключение датчика приближения
bool APDS9930_Sensor::enableProximity(bool enable) {
    if (!initialized) {
        return false;
    }

    if (enable) {
        // Настройка proximity
        if (!writeByte(APDS9930_PPULSE, APDS9930_DEFAULT_PPULSE)) {
            return false;
        }
    }

    return setMode(APDS9930_PEN, enable);
}

// Включение/выключение датчика освещенности
bool APDS9930_Sensor::enableLightSensor(bool enable) {
    if (!initialized) {
        return false;
    }

    return setMode(APDS9930_AEN, enable);
}