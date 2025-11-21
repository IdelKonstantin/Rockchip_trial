#include "APDS9960_Sensor.h"
#include <stdexcept>
#include <cmath>

// Конструктор
APDS9960_Sensor::APDS9960_Sensor(const std::string& i2c_device, uint8_t address) 
    : i2c_fd(-1), i2c_addr(address), initialized(false), 
      i2c_device(i2c_device), proximity_threshold(PROXIMITY_THRESHOLD_10CM), 
      current_proximity(0) {
    
    current_data = {false};
}

// Деструктор
APDS9960_Sensor::~APDS9960_Sensor() {
    if (i2c_fd >= 0) {
        // Выключение всех функций датчика
        writeByte(APDS9960_ENABLE, 0x00);
        close(i2c_fd);
    }
}

// Запись байта в регистр
bool APDS9960_Sensor::writeByte(uint8_t reg, uint8_t value) {
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
uint8_t APDS9960_Sensor::readByte(uint8_t reg) {
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

// Включение/выключение режимов работы
bool APDS9960_Sensor::setMode(uint8_t mode, bool enable) {
    uint8_t reg_val = readByte(APDS9960_ENABLE);
    
    if (enable) {
        reg_val |= mode;
    } else {
        reg_val &= ~mode;
    }
    
    return writeByte(APDS9960_ENABLE, reg_val);
}

// Настройка датчика приближения
bool APDS9960_Sensor::setupProximitySensor() {
    // Настройка времени интеграции proximity (2.78ms)
    if (!writeByte(APDS9960_PPULSE, 0x87)) { // 16 pulses, 16us
        return false;
    }

    // Настройка контроля proximity
    if (!writeByte(APDS9960_CONTROL, 0x20)) { // 1x proximity gain, 100mA LED drive
        return false;
    }

    // Настройка persistence (2 consecutive out-of-range)
    if (!writeByte(APDS9960_PERS, 0x11)) {
        return false;
    }

    // Настройка конфигурации 2
    if (!writeByte(APDS9960_CONFIG2, 0x01)) { // No saturation interrupts
        return false;
    }

    // Настройка конфигурации 3
    if (!writeByte(APDS9960_CONFIG3, 0x00)) { // No mask, all photodiodes active
        return false;
    }

    return true;
}

// Обновление статуса приближения
void APDS9960_Sensor::updateProximityStatus() {
    current_proximity = readByte(APDS9960_PDATA);
    current_data.proximity = (current_proximity > proximity_threshold);
}

// Инициализация датчика
bool APDS9960_Sensor::init() {
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
    uint8_t id = readByte(APDS9960_ID);
    if (id != APDS9960_ID_VALUE) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Выключаем все функции для чистой настройки
    if (!writeByte(APDS9960_ENABLE, 0x00)) {
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

    // Включение питания
    if (!setMode(APDS9960_PON, true)) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    usleep(10000); // 10ms задержка

    // Включение датчика приближения
    if (!setMode(APDS9960_PEN, true)) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Задержка для первого измерения
    usleep(50000); // 50ms

    // Тестовое чтение для проверки работоспособности
    uint8_t test_proximity = readByte(APDS9960_PDATA);
    
    if (test_proximity == 0xFF) { // Ошибка чтения
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    initialized = true;
    return true;
}

// Получение статуса приближения
const proxy::data& APDS9960_Sensor::getProximityStatus() {
    if (initialized) {
        updateProximityStatus();
    }
    return current_data;
}

// Идентификация датчика
const std::string APDS9960_Sensor::whoAmI() const {
    return "APDS-9960";
}

// Установка дистанции срабатывания
void APDS9960_Sensor::setDetectionDistance(int distance_cm) {
    if (distance_cm <= 0) {
        return;
    }

    // Эмпирическое преобразование см в пороговое значение
    // APDS-9960 возвращает значения от 0 (далеко) до 255 (близко)
    if (distance_cm <= 5) {
        proximity_threshold = PROXIMITY_THRESHOLD_5CM;   // Близко
    } else if (distance_cm <= 10) {
        proximity_threshold = PROXIMITY_THRESHOLD_10CM;  // Средняя дистанция
    } else if (distance_cm <= 15) {
        proximity_threshold = PROXIMITY_THRESHOLD_15CM;  // Дальняя дистанция
    } else {
        proximity_threshold = PROXIMITY_THRESHOLD_20CM;  // Очень дальняя
    }

    // Дополнительная настройка параметров в зависимости от дистанции
    uint8_t led_drive = 0x00; // LED drive current
    uint8_t proximity_gain = 0x00; // Proximity gain

    if (distance_cm <= 5) {
        led_drive = 0x00;      // 100mA (меньше ток для близких дистанций)
        proximity_gain = 0x00; // 1x gain
    } else if (distance_cm <= 10) {
        led_drive = 0x01;      // 50mA
        proximity_gain = 0x01; // 2x gain
    } else if (distance_cm <= 15) {
        led_drive = 0x02;      // 25mA
        proximity_gain = 0x02; // 4x gain
    } else {
        led_drive = 0x03;      // 12.5mA (больше ток для дальних дистанций)
        proximity_gain = 0x03; // 8x gain
    }

    // Обновление настроек датчика
    if (initialized) {
        uint8_t control_reg = (led_drive << 6) | (proximity_gain << 2);
        writeByte(APDS9960_CONTROL, control_reg);
    }
}

// Получение сырого значения приближения
uint8_t APDS9960_Sensor::getRawProximity() {
    if (initialized) {
        updateProximityStatus();
        return current_proximity;
    }
    return 0;
}

// Включение/выключение датчика жестов
bool APDS9960_Sensor::enableGestureSensor(bool enable) {
    if (!initialized) {
        return false;
    }

    if (enable) {
        // Настройка жестов
        if (!writeByte(APDS9960_GPULSE, 0x89)) { // 16 pulses, 16us
            return false;
        }
        if (!writeByte(APDS9960_GCONF3, 0x00)) { // All photodiodes active during gesture
            return false;
        }
        if (!writeByte(APDS9960_GCONF2, 0x01)) { // Gained 4x for gesture
            return false;
        }
    }

    return setMode(APDS9960_GEN, enable);
}

// Включение/выключение датчика освещенности
bool APDS9960_Sensor::enableLightSensor(bool enable) {
    if (!initialized) {
        return false;
    }
    return setMode(APDS9960_AEN, enable);
}