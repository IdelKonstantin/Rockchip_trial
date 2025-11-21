#include "MAX44009_Sensor.h"
#include <stdexcept>
#include <cmath>
#include <algorithm>

// Конструктор
MAX44009_Sensor::MAX44009_Sensor(const std::string& i2c_device, uint8_t address) 
    : i2c_fd(-1), i2c_addr(address), initialized(false), 
      i2c_device(i2c_device), proximity_threshold(PROXIMITY_THRESHOLD_10CM),
      baseline_lux(0.0f), current_lux(0.0f), baseline_calibrated(false) {
    
    current_data = {false};
}

// Деструктор
MAX44009_Sensor::~MAX44009_Sensor() {
    if (i2c_fd >= 0) {
        close(i2c_fd);
    }
}

// Запись байта в регистр
bool MAX44009_Sensor::writeByte(uint8_t reg, uint8_t value) {
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
uint8_t MAX44009_Sensor::readByte(uint8_t reg) {
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

// Расчет освещенности в люксах
float MAX44009_Sensor::calculateLux(uint8_t high_byte, uint8_t low_byte) {
    // Формат данных MAX44009:
    // High byte: [ exponent (4 bits) | mantissa high (4 bits) ]
    // Low byte:  [ mantissa low (4 bits) | 0000 ]
    
    uint8_t exponent = (high_byte >> 4) & 0x0F;
    uint8_t mantissa = ((high_byte & 0x0F) << 4) | (low_byte >> 4);
    
    // Расчет люксов: lux = 2^exponent × mantissa × 0.045
    double lux = pow(2.0, exponent) * mantissa * 0.045;
    
    return static_cast<float>(lux);
}

// Конфигурация датчика
bool MAX44009_Sensor::configureSensor() {
    // Настройка continuous mode для постоянного мониторинга
    uint8_t config = 0x80; // Continuous mode
    
    // Manual mode disabled (автоматический выбор времени интеграции)
    config &= ~0x40;
    
    // CDR disabled (no current division)
    config &= ~0x08;
    
    // Integration time = 800ms (максимальная чувствительность)
    config |= 0x03;
    
    return writeByte(MAX44009_CONFIG, config);
}

// Калибровка базового уровня освещенности
bool MAX44009_Sensor::calibrateBaseline() {
    if (!initialized) {
        return false;
    }

    // Измеряем несколько раз для стабильности
    float sum_lux = 0.0f;
    int samples = 5;
    
    for (int i = 0; i < samples; i++) {
        uint8_t lux_high = readByte(MAX44009_LUX_HIGH);
        uint8_t lux_low = readByte(MAX44009_LUX_LOW);
        sum_lux += calculateLux(lux_high, lux_low);
        usleep(100000); // 100ms между измерениями
    }
    
    baseline_lux = sum_lux / samples;
    baseline_calibrated = (baseline_lux > 0.1f); // Минимальный уровень для калибровки
    
    return baseline_calibrated;
}

// Обновление статуса приближения
void MAX44009_Sensor::updateProximityStatus() {
    if (!initialized || !baseline_calibrated) {
        return;
    }

    // Чтение текущей освещенности
    uint8_t lux_high = readByte(MAX44009_LUX_HIGH);
    uint8_t lux_low = readByte(MAX44009_LUX_LOW);
    current_lux = calculateLux(lux_high, lux_low);

    // Обнаружение приближения по падению освещенности
    if (baseline_lux > 0.1f) { // Минимальный уровень для детектирования
        float light_ratio = current_lux / baseline_lux;
        current_data.proximity = (light_ratio < (1.0f - proximity_threshold));
    } else {
        current_data.proximity = false;
    }
}

// Инициализация датчика
bool MAX44009_Sensor::init() {
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

    // Проверяем доступность датчика
    uint8_t test_config = readByte(MAX44009_CONFIG);
    if (test_config == 0xFF) { // Ошибка чтения
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Настройка датчика
    if (!configureSensor()) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Задержка для стабилизации
    usleep(200000); // 200ms

    // Калибровка базового уровня
    if (!calibrateBaseline()) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    initialized = true;
    return true;
}

// Получение статуса приближения
const proxy::data& MAX44009_Sensor::getProximityStatus() {
    if (initialized && baseline_calibrated) {
        updateProximityStatus();
    }
    return current_data;
}

// Идентификация датчика
const std::string MAX44009_Sensor::whoAmI() const {
    return "MAX44009";
}

// Установка дистанции срабатывания
void MAX44009_Sensor::setDetectionDistance(int distance_cm) {
    if (distance_cm <= 0) {
        return;
    }

    // Эмпирическое преобразование см в порог изменения освещенности
    if (distance_cm <= 5) {
        proximity_threshold = PROXIMITY_THRESHOLD_5CM;   // Чувствительно - 70% падение
    } else if (distance_cm <= 10) {
        proximity_threshold = PROXIMITY_THRESHOLD_10CM;  // Средняя - 50% падение
    } else if (distance_cm <= 15) {
        proximity_threshold = PROXIMITY_THRESHOLD_15CM;  // Менее чувствительно - 30% падение
    } else {
        proximity_threshold = PROXIMITY_THRESHOLD_20CM;  // Малочувствительно - 20% падение
    }
}

// Получение текущей освещенности
float MAX44009_Sensor::getCurrentLux() {
    return current_lux;
}

// Получение базовой освещенности
float MAX44009_Sensor::getBaselineLux() {
    return baseline_lux;
}

// Перекалибровка базового уровня
void MAX44009_Sensor::recalibrateBaseline() {
    if (initialized) {
        calibrateBaseline();
    }
}

// Проверка калибровки
bool MAX44009_Sensor::isBaselineCalibrated() const {
    return baseline_calibrated;
}