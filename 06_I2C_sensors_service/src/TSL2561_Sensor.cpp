#include "TSL2561_Sensor.h"
#include <stdexcept>
#include <cmath>

// Конструктор
TSL2561_Sensor::TSL2561_Sensor(const std::string& i2c_device, uint8_t address,
                               uint8_t integration, uint8_t gain_setting) 
    : i2c_fd(-1), i2c_addr(address), integration_time(integration), 
      gain(gain_setting), initialized(false), i2c_device(i2c_device) {
    
    current_data = {0.0f, light::level::TOTAL_DARKNESS};
}

// Деструктор
TSL2561_Sensor::~TSL2561_Sensor() {
    if (i2c_fd >= 0) {
        disable();
        close(i2c_fd);
    }
}

// Запись байта в регистр
bool TSL2561_Sensor::writeByte(uint8_t reg, uint8_t value) {
    if (i2c_fd < 0) {
        return false;
    }

    uint8_t data[2] = {static_cast<uint8_t>(TSL2561_CMD | reg), value};
    if (write(i2c_fd, data, 2) != 2) {
        return false;
    }
    
    usleep(10000);
    return true;
}

// Чтение байта из регистра
uint8_t TSL2561_Sensor::readByte(uint8_t reg) {
    if (i2c_fd < 0) {
        return 0;
    }

    uint8_t cmd = TSL2561_CMD | reg;
    if (write(i2c_fd, &cmd, 1) != 1) {
        return 0;
    }

    uint8_t value;
    if (read(i2c_fd, &value, 1) != 1) {
        return 0;
    }

    return value;
}

// Чтение слова (2 байта) из регистра
uint16_t TSL2561_Sensor::readWord(uint8_t reg) {
    if (i2c_fd < 0) {
        return 0;
    }

    uint8_t cmd = TSL2561_CMD | reg;
    if (write(i2c_fd, &cmd, 1) != 1) {
        return 0;
    }

    uint8_t data[2];
    if (read(i2c_fd, data, 2) != 2) {
        return 0;
    }

    return (data[1] << 8) | data[0];
}

// Включение датчика
bool TSL2561_Sensor::enable() {
    return writeByte(TSL2561_CONTROL, TSL2561_POWER_ON);
}

// Выключение датчика
bool TSL2561_Sensor::disable() {
    return writeByte(TSL2561_CONTROL, TSL2561_POWER_OFF);
}

// Расчет освещенности в люксах
light::lux_t TSL2561_Sensor::calculateLux(uint16_t ch0, uint16_t ch1) {
    // Коэффициенты для расчета (из даташита)
    constexpr double D = 0.6;  // Коэффициент для соотношения каналов
    constexpr double B = 1.67; // Коэффициент масштабирования
    constexpr double M = 1.0;  // Коэффициент масштабирования
    constexpr double R = 0.55; // Коэффициент для ИК-компенсации

    if (ch0 == 0) {
        return 0.0f;
    }

    double ratio = static_cast<double>(ch1) / static_cast<double>(ch0);
    double lux = 0.0;

    if (ratio <= D) {
        lux = (B * ch0 - M * ch1);
    } else {
        lux = (R * ch0 - ch1);
    }

    // Корректировка на время интеграции и усиление
    double integration_factor = 1.0;
    switch (integration_time) {
        case TSL2561_INTEGRATIONTIME_13MS:
            integration_factor = 322.0 / 11.0;
            break;
        case TSL2561_INTEGRATIONTIME_101MS:
            integration_factor = 322.0 / 81.0;
            break;
        case TSL2561_INTEGRATIONTIME_402MS:
            integration_factor = 1.0;
            break;
    }

    double gain_factor = (gain == TSL2561_GAIN_16X) ? 0.0625 : 1.0;

    lux = lux * integration_factor * gain_factor;

    return static_cast<light::lux_t>(lux > 0 ? lux : 0);
}

// Определение уровня освещенности на основе люксов
light::level TSL2561_Sensor::calculateLightLevel(light::lux_t lux) {
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
bool TSL2561_Sensor::init() {
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
    uint8_t id = readByte(TSL2561_ID);
    if ((id & 0xF0) != 0x10) { // TSL2561 имеет ID 0x10, 0x11, 0x12, 0x13
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Настраиваем время интеграции и усиление
    if (!writeByte(TSL2561_TIMING, integration_time | gain)) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Включаем датчик
    if (!enable()) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Задержка для первого измерения
    unsigned int delay_ms = 0;
    switch (integration_time) {
        case TSL2561_INTEGRATIONTIME_13MS:
            delay_ms = 15;
            break;
        case TSL2561_INTEGRATIONTIME_101MS:
            delay_ms = 120;
            break;
        case TSL2561_INTEGRATIONTIME_402MS:
            delay_ms = 450;
            break;
    }
    usleep(delay_ms * 1000);

    // Тестовое чтение для проверки работоспособности
    uint16_t test_ch0 = readWord(TSL2561_DATA0LOW);
    uint16_t test_ch1 = readWord(TSL2561_DATA1LOW);
    
    if (test_ch0 == 0 && test_ch1 == 0) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    initialized = true;
    return true;
}

// Получение данных об освещенности
const light::data& TSL2561_Sensor::getLightData() {
    if (!initialized) {
        return current_data;
    }

    // Чтение данных с двух каналов
    uint16_t ch0 = readWord(TSL2561_DATA0LOW); // Видимый + ИК
    uint16_t ch1 = readWord(TSL2561_DATA1LOW); // Только ИК

    if (ch0 != 0 || ch1 != 0) {
        current_data.lightIntencity = calculateLux(ch0, ch1);
        current_data.lightLevel = calculateLightLevel(current_data.lightIntencity);
    }

    return current_data;
}

// Идентификация датчика
const std::string TSL2561_Sensor::whoAmI() const {
    return "TSL2561";
}

// Установка времени интеграции
void TSL2561_Sensor::setIntegrationTime(uint8_t time) {
    if (time <= TSL2561_INTEGRATIONTIME_402MS) {
        integration_time = time;
        if (initialized) {
            writeByte(TSL2561_TIMING, integration_time | gain);
        }
    }
}

// Установка усиления
void TSL2561_Sensor::setGain(uint8_t gain_setting) {
    if (gain_setting == TSL2561_GAIN_1X || gain_setting == TSL2561_GAIN_16X) {
        gain = gain_setting;
        if (initialized) {
            writeByte(TSL2561_TIMING, integration_time | gain);
        }
    }
}