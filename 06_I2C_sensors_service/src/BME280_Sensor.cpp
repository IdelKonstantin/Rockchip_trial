#include "BME280_Sensor.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cmath>
#include <stdexcept>

const std::string BME280_Sensor::whoAmI() const {

    return "BME280";
}

// Функции для работы с I2C
uint8_t BME280_Sensor::readByte(uint8_t reg) {
    if (write(i2c_fd, &reg, 1) != 1) {
        throw std::runtime_error("I2C write failed");
    }
    
    uint8_t value;
    if (read(i2c_fd, &value, 1) != 1) {
        throw std::runtime_error("I2C read failed");
    }
    return value;
}

uint16_t BME280_Sensor::readWord(uint8_t reg) {
    if (write(i2c_fd, &reg, 1) != 1) {
        throw std::runtime_error("I2C write failed");
    }
    
    uint8_t data[2];
    if (read(i2c_fd, data, 2) != 2) {
        throw std::runtime_error("I2C read failed");
    }
    return (data[1] << 8) | data[0];
}

int16_t BME280_Sensor::readWordSigned(uint8_t reg) {
    return static_cast<int16_t>(readWord(reg));
}

void BME280_Sensor::writeByte(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    if (write(i2c_fd, data, 2) != 2) {
        throw std::runtime_error("I2C write failed");
    }
}

// Чтение калибровочных данных
void BME280_Sensor::readCalibrationData() {
    // Температура
    calib.dig_T1 = readWord(REGISTER_DIG_T1);
    calib.dig_T2 = readWordSigned(REGISTER_DIG_T2);
    calib.dig_T3 = readWordSigned(REGISTER_DIG_T3);
    
    // Давление
    calib.dig_P1 = readWord(REGISTER_DIG_P1);
    calib.dig_P2 = readWordSigned(REGISTER_DIG_P2);
    calib.dig_P3 = readWordSigned(REGISTER_DIG_P3);
    calib.dig_P4 = readWordSigned(REGISTER_DIG_P4);
    calib.dig_P5 = readWordSigned(REGISTER_DIG_P5);
    calib.dig_P6 = readWordSigned(REGISTER_DIG_P6);
    calib.dig_P7 = readWordSigned(REGISTER_DIG_P7);
    calib.dig_P8 = readWordSigned(REGISTER_DIG_P8);
    calib.dig_P9 = readWordSigned(REGISTER_DIG_P9);
    
    // Влажность
    calib.dig_H1 = readByte(REGISTER_DIG_H1);
    calib.dig_H2 = readWordSigned(REGISTER_DIG_H2);
    calib.dig_H3 = readByte(REGISTER_DIG_H3);
    
    uint8_t h4_msb = readByte(0xE4);
    uint8_t h4_lsb = readByte(0xE5);
    uint8_t h5_msb = readByte(0xE6);
    uint8_t h5_lsb = readByte(0xE5);
    
    calib.dig_H4 = (h4_msb << 4) | (h4_lsb & 0x0F);
    calib.dig_H5 = (h5_msb << 4) | (h5_lsb >> 4);
    calib.dig_H6 = static_cast<int8_t>(readByte(REGISTER_DIG_H6));
}

// Инициализация датчика
void BME280_Sensor::initializeSensor() {
    // Сброс
    writeByte(REGISTER_SOFTRESET, 0xB6);
    usleep(10000);
    
    // Настройка влажности
    writeByte(REGISTER_CONTROLHUMID, 0x01);
    
    // Настройка температуры и давления
    writeByte(REGISTER_CONTROL, 0x27);
}

// Компенсация температуры
int32_t BME280_Sensor::compensateTemperature(int32_t adc_T) {
    int32_t var1, var2;
    
    var1 = ((((adc_T >> 3) - (static_cast<int32_t>(calib.dig_T1) << 1))) * 
            (static_cast<int32_t>(calib.dig_T2))) >> 11;
    
    var2 = (((((adc_T >> 4) - (static_cast<int32_t>(calib.dig_T1))) * 
             ((adc_T >> 4) - (static_cast<int32_t>(calib.dig_T1)))) >> 12) * 
             (static_cast<int32_t>(calib.dig_T3))) >> 14;
    
    t_fine = var1 + var2;
    return (t_fine * 5 + 128) >> 8;
}

// Компенсация давления
uint32_t BME280_Sensor::compensatePressure(int32_t adc_P) {
    int64_t var1, var2, p;
    
    var1 = (static_cast<int64_t>(t_fine)) - 128000;
    var2 = var1 * var1 * static_cast<int64_t>(calib.dig_P6);
    var2 = var2 + ((var1 * static_cast<int64_t>(calib.dig_P5)) << 17);
    var2 = var2 + ((static_cast<int64_t>(calib.dig_P4)) << 35);
    var1 = ((var1 * var1 * static_cast<int64_t>(calib.dig_P3)) >> 8) + 
           ((var1 * static_cast<int64_t>(calib.dig_P2)) << 12);
    var1 = ((((static_cast<int64_t>(1)) << 47) + var1)) * 
            (static_cast<int64_t>(calib.dig_P1)) >> 33;
    
    if (var1 == 0) {
        return 0;
    }
    
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = ((static_cast<int64_t>(calib.dig_P9)) * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((static_cast<int64_t>(calib.dig_P8)) * p) >> 19;
    
    p = ((p + var1 + var2) >> 8) + ((static_cast<int64_t>(calib.dig_P7)) << 4);
    return static_cast<uint32_t>(p) / 256;
}

// Компенсация влажности
uint32_t BME280_Sensor::compensateHumidity(int32_t adc_H) {
    int32_t v_x1_u32r;
    
    v_x1_u32r = (t_fine - (static_cast<int32_t>(76800)));
    v_x1_u32r = (((((adc_H << 14) - ((static_cast<int32_t>(calib.dig_H4)) << 20) - 
                   ((static_cast<int32_t>(calib.dig_H5)) * v_x1_u32r)) + 
                   (static_cast<int32_t>(16384))) >> 15) * 
                 (((((((v_x1_u32r * (static_cast<int32_t>(calib.dig_H6))) >> 10) * 
                 (((v_x1_u32r * (static_cast<int32_t>(calib.dig_H3))) >> 11) + 
                 (static_cast<int32_t>(32768)))) >> 10) + 
                 (static_cast<int32_t>(2097152))) * 
                 (static_cast<int32_t>(calib.dig_H2)) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * 
                 (static_cast<int32_t>(calib.dig_H1))) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    
    return static_cast<uint32_t>(v_x1_u32r >> 12);
}

// Чтение данных с датчика
bool BME280_Sensor::readSensorData() {
    uint8_t reg = REGISTER_PRESSUREDATA;
    uint8_t sensor_data[8];
    
    // Читаем данные давления и температуры
    if (write(i2c_fd, &reg, 1) != 1) {
        return false;
    }
    
    if (read(i2c_fd, sensor_data, 8) != 8) {
        return false;
    }
    
    int32_t adc_P = (sensor_data[0] << 12) | (sensor_data[1] << 4) | (sensor_data[2] >> 4);
    int32_t adc_T = (sensor_data[3] << 12) | (sensor_data[4] << 4) | (sensor_data[5] >> 4);
    
    // Читаем данные влажности
    reg = REGISTER_HUMIDDATA;
    uint8_t hum_data[2];
    
    if (write(i2c_fd, &reg, 1) != 1) {
        return false;
    }
    
    if (read(i2c_fd, hum_data, 2) != 2) {
        return false;
    }
    
    int32_t adc_H = (hum_data[0] << 8) | hum_data[1];
    
    // Компенсируем данные
    int32_t t = compensateTemperature(adc_T);
    uint32_t p = compensatePressure(adc_P);
    uint32_t h = compensateHumidity(adc_H);
    
    // Обновляем структуру данных согласно интерфейсу
    current_data.temperature = t / 100.0f;
    current_data.pressure = static_cast<uint16_t>(p / 100.0f); // Приведение к press_t
    current_data.humidity = static_cast<uint8_t>(h / 1024.0f); // Приведение к humid_t
    
    // BME280 не поддерживает измерение скорости и направления ветра
    current_data.windSpeed = 0.0f;
    current_data.windDirection = 0;
    
    return true;
}

// Конструктор
BME280_Sensor::BME280_Sensor(const std::string& i2c_device, uint8_t address) 
    : i2c_fd(-1), i2c_addr(address), i2c_device(i2c_device) {}

// Деструктор
BME280_Sensor::~BME280_Sensor() {
    if (i2c_fd >= 0) {
        close(i2c_fd);
    }
}

// Инициализация датчика
bool BME280_Sensor::init() {
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
    
    // Проверяем ID устройства
    if (readByte(REGISTER_CHIPID) != 0x60) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }
    
    // Читаем калибровочные данные
    readCalibrationData();
    
    // Инициализируем датчик
    initializeSensor();
    
    // Читаем начальные данные
    if (!readSensorData()) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }
    
    initialized = true;
    return true;
}

// Получение метеоданных
const meteo::data& BME280_Sensor::getMeteoConditions() {
    if (initialized) {
        readSensorData(); // Обновляем данные при каждом запросе
    }
    return current_data;
}