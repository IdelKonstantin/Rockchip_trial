#include "VCNL4040_Sensor.h"
#include <stdexcept>
#include <cmath>

// Конструктор
VCNL4040_Sensor::VCNL4040_Sensor(const std::string& i2c_device, uint8_t address) 
    : i2c_fd(-1), i2c_addr(address), initialized(false), 
      i2c_device(i2c_device), proximity_threshold(PROXIMITY_THRESHOLD_10CM), 
      current_proximity(0) {
    
    current_data = {false};
}

// Деструктор
VCNL4040_Sensor::~VCNL4040_Sensor() {
    if (i2c_fd >= 0) {
        // Выключение датчика приближения
        enableProximitySensor(false);
        close(i2c_fd);
    }
}

// Запись байта в регистр
bool VCNL4040_Sensor::writeByte(uint8_t reg, uint8_t value) {
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

// Запись слова (2 байта) в регистр
bool VCNL4040_Sensor::writeWord(uint8_t reg, uint16_t value) {
    if (i2c_fd < 0) {
        return false;
    }

    uint8_t data[3] = {reg, static_cast<uint8_t>(value & 0xFF), static_cast<uint8_t>((value >> 8) & 0xFF)};
    if (write(i2c_fd, data, 3) != 3) {
        return false;
    }
    
    usleep(10000);
    return true;
}

// Чтение байта из регистра
uint8_t VCNL4040_Sensor::readByte(uint8_t reg) {
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
uint16_t VCNL4040_Sensor::readWord(uint8_t reg_low) {
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

// Настройка датчика приближения
bool VCNL4040_Sensor::setupProximitySensor() {
    // Конфигурация 1: Включение proximity, 8T, smart persistence
    // PS_SD = 0 (power on), PS_HD = 0 (16-bit output), PS_IT = 1T (default)
    if (!writeWord(VCNL4040_PS_CONF1, 0x0001)) {
        return false;
    }

    // Конфигурация 2: PS_INT disabled, smart persistence enabled
    if (!writeWord(VCNL4040_PS_CONF2, 0x0001)) {
        return false;
    }

    // Конфигурация 3: PS_AF = 0 (auto mode off), PS_TRIG = 0 (no trigger), PS_SC_EN = 0
    if (!writeWord(VCNL4040_PS_CONF3, 0x0000)) {
        return false;
    }

    // Установка тока LED: 50mA (хороший баланс между дальностью и потреблением)
    if (!setProximityLEDCurrent(2)) { // 50mA
        return false;
    }

    // Установка времени интеграции: 1T (быстрое измерение)
    if (!setProximityIntegrationTime(0)) {
        return false;
    }

    return true;
}

// Установка времени интеграции proximity
bool VCNL4040_Sensor::setProximityIntegrationTime(uint8_t time) {
    if (time > 3) { // Допустимые значения: 0-3
        return false;
    }

    uint16_t conf1 = readWord(VCNL4040_PS_CONF1);
    conf1 &= ~(0x03 << 1); // Очищаем биты PS_IT
    conf1 |= (time & 0x03) << 1;
    
    return writeWord(VCNL4040_PS_CONF1, conf1);
}

// Установка тока LED
bool VCNL4040_Sensor::setProximityLEDCurrent(uint8_t current) {
    if (current > 3) { // Допустимые значения: 0-3
        return false;
    }

    uint16_t conf1 = readWord(VCNL4040_PS_CONF1);
    conf1 &= ~(0x03 << 11); // Очищаем биты PS_LED_I
    conf1 |= (current & 0x03) << 11;
    
    return writeWord(VCNL4040_PS_CONF1, conf1);
}

// Обновление статуса приближения
void VCNL4040_Sensor::updateProximityStatus() {
    current_proximity = readWord(VCNL4040_PS_DATA_L);
    current_data.proximity = (current_proximity > proximity_threshold);
}

// Инициализация датчика
bool VCNL4040_Sensor::init() {
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
    uint16_t id = readWord(VCNL4040_ID_L);
    if (id != VCNL4040_DEVICE_ID) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Настройка датчика приближения
    if (!setupProximitySensor()) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Включение датчика приближения
    if (!enableProximitySensor(true)) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Задержка для первого измерения
    usleep(100000); // 100ms

    // Тестовое чтение для проверки работоспособности
    uint16_t test_proximity = readWord(VCNL4040_PS_DATA_L);
    
    if (test_proximity == 0xFFFF) { // Ошибка чтения
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    initialized = true;
    return true;
}

// Получение статуса приближения
const proxy::data& VCNL4040_Sensor::getProximityStatus() {
    if (initialized) {
        updateProximityStatus();
    }
    return current_data;
}

// Идентификация датчика
const std::string VCNL4040_Sensor::whoAmI() const {
    return "VCNL4040";
}

// Установка дистанции срабатывания
void VCNL4040_Sensor::setDetectionDistance(int distance_cm) {
    if (distance_cm <= 0) {
        return;
    }

    // Эмпирическое преобразование см в пороговое значение
    // VCNL4040 возвращает значения от 0 (далеко) до 4095 (близко)
    if (distance_cm <= 5) {
        proximity_threshold = PROXIMITY_THRESHOLD_5CM;   // Близко (высокая чувствительность)
    } else if (distance_cm <= 10) {
        proximity_threshold = PROXIMITY_THRESHOLD_10CM;  // Средняя дистанция
    } else if (distance_cm <= 15) {
        proximity_threshold = PROXIMITY_THRESHOLD_15CM;  // Дальняя дистанция
    } else {
        proximity_threshold = PROXIMITY_THRESHOLD_20CM;  // Очень дальняя (низкая чувствительность)
    }

    // Настройка параметров датчика в зависимости от дистанции
    uint8_t led_current = 0;
    uint8_t integration_time = 0;

    if (distance_cm <= 5) {
        led_current = 0;        // 50mA (стандарт для близких дистанций)
        integration_time = 0;   // 1T (быстро)
    } else if (distance_cm <= 10) {
        led_current = 1;        // 75mA
        integration_time = 1;   // 1.5T
    } else if (distance_cm <= 15) {
        led_current = 2;        // 100mA
        integration_time = 2;   // 2T
    } else {
        led_current = 3;        // 120mA (максимум для дальних дистанций)
        integration_time = 3;   // 3T (максимальная чувствительность)
    }

    // Обновление настроек датчика
    if (initialized) {
        setProximityLEDCurrent(led_current);
        setProximityIntegrationTime(integration_time);
        
        // Небольшая задержка после изменения настроек
        usleep(50000); // 50ms
    }
}

// Получение сырого значения приближения
uint16_t VCNL4040_Sensor::getRawProximity() {
    if (initialized) {
        updateProximityStatus();
        return current_proximity;
    }
    return 0;
}

// Включение/выключение датчика приближения
bool VCNL4040_Sensor::enableProximitySensor(bool enable) {
    if (!initialized && !enable) {
        return false; // Нельзя выключить если не инициализирован
    }

    uint16_t conf1 = readWord(VCNL4040_PS_CONF1);
    
    if (enable) {
        conf1 &= ~(1 << 0); // PS_SD = 0 (power on)
    } else {
        conf1 |= (1 << 0);  // PS_SD = 1 (shutdown)
    }
    
    return writeWord(VCNL4040_PS_CONF1, conf1);
}

// Получение уровня окружающего освещения
uint16_t VCNL4040_Sensor::getAmbientLight() {
    if (!initialized) {
        return 0;
    }
    
    // ALS_DATA регистры: 0x05 (L), 0x06 (H)
    return readWord(0x05);
}