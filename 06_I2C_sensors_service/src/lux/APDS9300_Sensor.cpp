#include "APDS9300_Sensor.h"
#include <stdexcept>
#include <cmath>

// Конструктор
APDS9300_Sensor::APDS9300_Sensor(const std::string& i2c_device, uint8_t address,
                               uint8_t integration, uint8_t gain_setting) 
    : i2c_fd(-1), i2c_addr(address), integration_time(integration), 
      gain(gain_setting), initialized(false), i2c_device(i2c_device) {
    
    current_data = {0.0f, light::level::TOTAL_DARKNESS};
}

// Деструктор
APDS9300_Sensor::~APDS9300_Sensor() {
    if (i2c_fd >= 0) {
        disable();
        close(i2c_fd);
    }
}

// Запись байта в регистр
bool APDS9300_Sensor::writeByte(uint8_t reg, uint8_t value) {
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
uint8_t APDS9300_Sensor::readByte(uint8_t reg) {
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
uint16_t APDS9300_Sensor::readWord(uint8_t reg_low) {
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

// Включение датчика
bool APDS9300_Sensor::enable() {
    return writeByte(APDS9300_CONTROL, APDS9300_POWER_ON);
}

// Выключение датчика
bool APDS9300_Sensor::disable() {
    return writeByte(APDS9300_CONTROL, APDS9300_POWER_OFF);
}

// Расчет освещенности в люксах
light::lux_t APDS9300_Sensor::calculateLux(uint16_t ch0, uint16_t ch1) {
    if (ch0 == 0) {
        return 0.0f;
    }

    // Расчет соотношения IR/Visible
    double ratio = static_cast<double>(ch1) / static_cast<double>(ch0);
    
    // Эмпирические коэффициенты для APDS-9300 (из даташита)
    double lux = 0.0;
    
    if (ratio <= 0.50) {
        lux = (0.0304 * ch0) - (0.062 * ch0 * pow(ratio, 1.4));
    } else if (ratio <= 0.61) {
        lux = (0.0224 * ch0) - (0.031 * ch1);
    } else if (ratio <= 0.80) {
        lux = (0.0128 * ch0) - (0.0153 * ch1);
    } else if (ratio <= 1.30) {
        lux = (0.00146 * ch0) - (0.00112 * ch1);
    } else {
        lux = 0;
    }

    // Корректировка на время интеграции
    double integration_factor = 1.0;
    switch (integration_time) {
        case APDS9300_INTEGRATIONTIME_13MS:
            integration_factor = 322.0 / 11.0;
            break;
        case APDS9300_INTEGRATIONTIME_101MS:
            integration_factor = 322.0 / 81.0;
            break;
        case APDS9300_INTEGRATIONTIME_402MS:
            integration_factor = 1.0;
            break;
    }

    // Корректировка на усиление
    double gain_factor = (gain == APDS9300_GAIN_16X) ? 0.0625 : 1.0;

    lux = lux * integration_factor * gain_factor;

    return static_cast<light::lux_t>(lux > 0 ? lux : 0);
}

// Определение уровня освещенности на основе люксов
light::level APDS9300_Sensor::calculateLightLevel(light::lux_t lux) {
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
bool APDS9300_Sensor::init() {
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
    uint8_t id = getDeviceID();
    if (id != APDS9300_ID_VALUE) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    // Настраиваем время интеграции и усиление
    if (!writeByte(APDS9300_TIMING, integration_time | gain)) {
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
        case APDS9300_INTEGRATIONTIME_13MS:
            delay_ms = 15;
            break;
        case APDS9300_INTEGRATIONTIME_101MS:
            delay_ms = 120;
            break;
        case APDS9300_INTEGRATIONTIME_402MS:
            delay_ms = 450;
            break;
    }
    usleep(delay_ms * 1000);

    // Тестовое чтение для проверки работоспособности
    uint16_t test_ch0 = readWord(APDS9300_DATA0LOW);
    uint16_t test_ch1 = readWord(APDS9300_DATA1LOW);
    
    if (test_ch0 == 0 && test_ch1 == 0) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }

    initialized = true;
    return true;
}

// Получение данных об освещенности
const light::data& APDS9300_Sensor::getLightData() {
    if (!initialized) {
        return current_data;
    }

    // Чтение данных с двух каналов
    uint16_t ch0 = readWord(APDS9300_DATA0LOW); // Видимый + ИК
    uint16_t ch1 = readWord(APDS9300_DATA1LOW); // Только ИК

    if (ch0 != 0 || ch1 != 0) {
        current_data.lightIntencity = calculateLux(ch0, ch1);
        current_data.lightLevel = calculateLightLevel(current_data.lightIntencity);
    }

    return current_data;
}

// Идентификация датчика
const std::string APDS9300_Sensor::whoAmI() const {
    return "APDS-9300";
}

// Получение ID устройства
uint8_t APDS9300_Sensor::getDeviceID() {
    if (i2c_fd < 0) {
        return 0;
    }
    return readByte(APDS9300_ID);
}

// Установка времени интеграции
void APDS9300_Sensor::setIntegrationTime(uint8_t time) {
    if (time <= APDS9300_INTEGRATIONTIME_402MS) {
        integration_time = time;
        if (initialized) {
            writeByte(APDS9300_TIMING, integration_time | gain);
            
            // Задержка после изменения времени интеграции
            unsigned int delay_ms = 0;
            switch (integration_time) {
                case APDS9300_INTEGRATIONTIME_13MS:
                    delay_ms = 15;
                    break;
                case APDS9300_INTEGRATIONTIME_101MS:
                    delay_ms = 120;
                    break;
                case APDS9300_INTEGRATIONTIME_402MS:
                    delay_ms = 450;
                    break;
            }
            usleep(delay_ms * 1000);
        }
    }
}

// Установка усиления
void APDS9300_Sensor::setGain(uint8_t gain_setting) {
    if (gain_setting == APDS9300_GAIN_1X || gain_setting == APDS9300_GAIN_16X) {
        gain = gain_setting;
        if (initialized) {
            writeByte(APDS9300_TIMING, integration_time | gain);
        }
    }
}