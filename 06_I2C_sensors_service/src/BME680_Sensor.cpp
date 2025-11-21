#include "BME680_Sensor.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cmath>
#include <stdexcept>

const std::string BME680_Sensor::whoAmI() const {

    return "BME680";
}

// Функции для работы с I2C
uint8_t BME680_Sensor::readByte(uint8_t reg) {
    if (write(i2c_fd, &reg, 1) != 1) {
        throw std::runtime_error("I2C write failed");
    }
    
    uint8_t value;
    if (read(i2c_fd, &value, 1) != 1) {
        throw std::runtime_error("I2C read failed");
    }
    return value;
}

uint16_t BME680_Sensor::readWord(uint8_t reg) {
    if (write(i2c_fd, &reg, 1) != 1) {
        throw std::runtime_error("I2C write failed");
    }
    
    uint8_t data[2];
    if (read(i2c_fd, data, 2) != 2) {
        throw std::runtime_error("I2C read failed");
    }
    return (data[1] << 8) | data[0];
}

int16_t BME680_Sensor::readWordSigned(uint8_t reg) {
    return static_cast<int16_t>(readWord(reg));
}

int8_t BME680_Sensor::readByteSigned(uint8_t reg) {
    return static_cast<int8_t>(readByte(reg));
}

void BME680_Sensor::writeByte(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    if (write(i2c_fd, data, 2) != 2) {
        throw std::runtime_error("I2C write failed");
    }
}

void BME680_Sensor::readBlock(uint8_t reg, uint8_t* data, uint8_t length) {
    if (write(i2c_fd, &reg, 1) != 1) {
        throw std::runtime_error("I2C write failed");
    }
    
    if (read(i2c_fd, data, length) != length) {
        throw std::runtime_error("I2C read failed");
    }
}

// Чтение калибровочных данных (только для температуры, давления, влажности)
void BME680_Sensor::readCalibrationData() {
    uint8_t calib_data[35];
    readBlock(REGISTER_CALIB_DATA, calib_data, 35);

    // Температура
    calib.par_t1 = (calib_data[33] << 8) | calib_data[32];
    calib.par_t2 = (calib_data[2] << 8) | calib_data[1];
    calib.par_t3 = static_cast<int8_t>(calib_data[3]);

    // Давление
    calib.par_p1 = (calib_data[6] << 8) | calib_data[5];
    calib.par_p2 = (calib_data[8] << 8) | calib_data[7];
    calib.par_p3 = static_cast<int8_t>(calib_data[9]);
    calib.par_p4 = (calib_data[12] << 8) | calib_data[11];
    calib.par_p5 = (calib_data[14] << 8) | calib_data[13];
    calib.par_p6 = static_cast<int8_t>(calib_data[16]);
    calib.par_p7 = static_cast<int8_t>(calib_data[15]);
    calib.par_p8 = (calib_data[20] << 8) | calib_data[19];
    calib.par_p9 = (calib_data[22] << 8) | calib_data[21];
    calib.par_p10 = static_cast<int8_t>(calib_data[23]);

    // Влажность
    calib.par_h1 = (calib_data[27] << 4) | (calib_data[26] & 0x0F);
    calib.par_h2 = (calib_data[25] << 4) | (calib_data[26] >> 4);
    calib.par_h3 = static_cast<int8_t>(calib_data[28]);
    calib.par_h4 = static_cast<int8_t>(calib_data[29]);
    calib.par_h5 = static_cast<int8_t>(calib_data[30]);
    calib.par_h6 = static_cast<int8_t>(calib_data[31]);
    calib.par_h7 = static_cast<int8_t>(calib_data[34]);
}

// Инициализация датчика (без газового сенсора)
void BME680_Sensor::initializeSensor() {
    // Сброс
    writeByte(REGISTER_RESET, 0xB6);
    usleep(10000);

    // Настройка влажности (oversampling x2)
    writeByte(REGISTER_CTRL_HUM, 0x02);

    // Настройка температуры и давления (oversampling x2), нормальный режим
    writeByte(REGISTER_CTRL_MEAS, 0x2B);
}

// Компенсация температуры
double BME680_Sensor::compensateTemperature(int32_t raw_temp) {
    int64_t var1, var2, var3;
    int32_t calc_temp;

    var1 = ((int64_t)raw_temp / 8) - ((int64_t)calib.par_t1 * 2);
    var2 = (var1 * (int64_t)calib.par_t2) / 2048;
    var3 = ((var1 / 2) * (var1 / 2)) / 4096;
    var3 = ((var3) * ((int64_t)calib.par_t3 * 16)) / 16384;

    t_fine = (int32_t)(var2 + var3);
    calc_temp = (int32_t)(((t_fine * 5) + 128) / 256);

    return calc_temp / 100.0;
}

// Компенсация давления
double BME680_Sensor::compensatePressure(int32_t raw_pressure) {
    int64_t var1, var2, var3, var4;
    int32_t calc_pres;

    var1 = ((int64_t)t_fine / 2) - 64000;
    var2 = ((((var1 / 4) * (var1 / 4)) / 2048) * (int64_t)calib.par_p6) / 4;
    var2 = var2 + ((var1 * (int64_t)calib.par_p5) * 2);
    var2 = (var2 / 4) + ((int64_t)calib.par_p4 * 65536);
    var1 = (((((var1 / 4) * (var1 / 4)) / 8192) * ((int64_t)calib.par_p3 * 32) / 8) + 
           ((int64_t)calib.par_p2 * var1)) / 2;
    var1 = var1 / 262144;
    var1 = ((32768 + var1) * (int64_t)calib.par_p1) / 32768;

    if (var1 == 0) {
        return 0;
    }

    var4 = (uint32_t)(1048576 - raw_pressure);
    var4 = (uint32_t)((var4 - (var2 / 4096)) * 3125);
    if (var4 >= 0x40000000) {
        var4 = ((uint32_t)var4 / (uint32_t)var1) * 2;
    } else {
        var4 = ((uint32_t)var4 * 2) / (uint32_t)var1;
    }
    var1 = (((int64_t)calib.par_p9) * (int64_t)(((var4 / 8) * (var4 / 8)) / 8192)) / 4096;
    var2 = ((int64_t)(var4 / 4) * (int64_t)calib.par_p8) / 8192;
    var3 = ((int64_t)(var4 / 256) * (int64_t)(var4 / 256) * (int64_t)(var4 / 256) * 
           (int64_t)calib.par_p10) / 131072;

    calc_pres = (int32_t)(var4 + ((var1 + var2 + var3 + (int64_t)calib.par_p7 * 128) / 16));

    return calc_pres / 100.0;
}

// Компенсация влажности
double BME680_Sensor::compensateHumidity(int32_t raw_humidity) {
    int32_t var1, var2, var3, var4, var5, var6;
    int32_t calc_hum;

    var1 = raw_humidity - ((int32_t)((calib.par_h1 * 16) + ((calib.par_h3 / 2) * t_fine)));
    var2 = var1 * (((int32_t)calib.par_h2 / 262144) * 
           (1 + ((calib.par_h4 / 16384) * t_fine) + ((calib.par_h5 / 1048576) * t_fine * t_fine)));
    var3 = calib.par_h6 / 16384;
    var4 = calib.par_h7 / 2097152;
    var5 = var2 + (var3 + var4 * t_fine) * var2 * var2;
    var6 = var5;
    if (var5 < 0) var6 = 0;
    if (var5 > 419430400) var6 = 419430400;

    calc_hum = (uint32_t)(var6 / 4096);

    return calc_hum / 1024.0;
}

// Чтение данных с датчика (только температура, давление, влажность)
bool BME680_Sensor::readSensorData() {
    uint8_t data[11];
    readBlock(REGISTER_DATA, data, 11);

    // Извлечение сырых данных (только температура, давление, влажность)
    uint32_t raw_pressure = (uint32_t)((data[2] << 12) | (data[3] << 4) | (data[4] >> 4));
    uint32_t raw_temp = (uint32_t)((data[5] << 12) | (data[6] << 4) | (data[7] >> 4));
    uint32_t raw_humidity = (uint16_t)((data[8] << 8) | data[9]);

    // Компенсация данных
    double temperature = compensateTemperature(raw_temp);
    double pressure = compensatePressure(raw_pressure);
    double humidity = compensateHumidity(raw_humidity);

    // Обновление структуры данных
    current_data.temperature = static_cast<float>(temperature);
    current_data.pressure = static_cast<uint16_t>(pressure); // hPa
    current_data.humidity = static_cast<uint8_t>(humidity);  // %
    
    // BME680 не поддерживает измерение ветра
    current_data.windSpeed = 0.0f;
    current_data.windDirection = 0;

    return true;
}

// Конструктор
BME680_Sensor::BME680_Sensor(const std::string& i2c_device, uint8_t address) 
    : i2c_fd(-1), i2c_addr(address), i2c_device(i2c_device) {}

// Деструктор
BME680_Sensor::~BME680_Sensor() {
    if (i2c_fd >= 0) {
        close(i2c_fd);
    }
}

// Инициализация датчика
bool BME680_Sensor::init() {
    if (initialized) {
        return true;
    }

    i2c_fd = open(i2c_device.c_str(), O_RDWR);
    if (i2c_fd < 0) {
        return false;
    }
    
    if (ioctl(i2c_fd, I2C_SLAVE, i2c_addr) < 0) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }
    
    if (readByte(REGISTER_CHIP_ID) != CHIP_ID_BME680) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }
    
    readCalibrationData();
    initializeSensor();
    
    if (!readSensorData()) {
        close(i2c_fd);
        i2c_fd = -1;
        return false;
    }
    
    initialized = true;
    return true;
}

// Получение метеоданных
const meteo::data& BME680_Sensor::getMeteoConditions() {
    if (initialized) {
        readSensorData();
    }
    return current_data;
}