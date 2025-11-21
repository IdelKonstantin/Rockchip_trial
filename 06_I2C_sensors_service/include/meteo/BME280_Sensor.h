#ifndef BME280_SENSOR_H
#define BME280_SENSOR_H

#include "iMeteoSensor.h"
#include <cstdint>
#include <string>

class BME280_Sensor : public iMeteoSensor {
private:
    // Структура для калибровочных данных BME280
    struct CalibData {
        uint16_t dig_T1;
        int16_t dig_T2, dig_T3;
        uint16_t dig_P1;
        int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
        uint8_t dig_H1, dig_H3;
        int16_t dig_H2, dig_H4, dig_H5, dig_H6;
    };

    // Регистры BME280
    static constexpr uint8_t REGISTER_DIG_T1        = 0x88;
    static constexpr uint8_t REGISTER_DIG_T2        = 0x8A;
    static constexpr uint8_t REGISTER_DIG_T3        = 0x8C;
    static constexpr uint8_t REGISTER_DIG_P1        = 0x8E;
    static constexpr uint8_t REGISTER_DIG_P2        = 0x90;
    static constexpr uint8_t REGISTER_DIG_P3        = 0x92;
    static constexpr uint8_t REGISTER_DIG_P4        = 0x94;
    static constexpr uint8_t REGISTER_DIG_P5        = 0x96;
    static constexpr uint8_t REGISTER_DIG_P6        = 0x98;
    static constexpr uint8_t REGISTER_DIG_P7        = 0x9A;
    static constexpr uint8_t REGISTER_DIG_P8        = 0x9C;
    static constexpr uint8_t REGISTER_DIG_P9        = 0x9E;
    static constexpr uint8_t REGISTER_DIG_H1        = 0xA1;
    static constexpr uint8_t REGISTER_DIG_H2        = 0xE1;
    static constexpr uint8_t REGISTER_DIG_H3        = 0xE3;
    static constexpr uint8_t REGISTER_DIG_H4        = 0xE4;
    static constexpr uint8_t REGISTER_DIG_H5        = 0xE5;
    static constexpr uint8_t REGISTER_DIG_H6        = 0xE7;
    static constexpr uint8_t REGISTER_CHIPID        = 0xD0;
    static constexpr uint8_t REGISTER_SOFTRESET     = 0xE0;
    static constexpr uint8_t REGISTER_CONTROLHUMID  = 0xF2;
    static constexpr uint8_t REGISTER_CONTROL       = 0xF4;
    static constexpr uint8_t REGISTER_PRESSUREDATA  = 0xF7;
    static constexpr uint8_t REGISTER_TEMPDATA      = 0xFA;
    static constexpr uint8_t REGISTER_HUMIDDATA     = 0xFD;

    int i2c_fd;
    uint8_t i2c_addr;
    CalibData calib;
    int32_t t_fine{0};
    meteo::data current_data{15.0f, 1013, 50, 4.0f, 90};
    bool initialized{false};
    std::string i2c_device;

    // Функции для работы с I2C
    uint8_t readByte(uint8_t reg);
    uint16_t readWord(uint8_t reg);
    int16_t readWordSigned(uint8_t reg);
    void writeByte(uint8_t reg, uint8_t value);

    // Внутренние методы
    void readCalibrationData();
    void initializeSensor();
    int32_t compensateTemperature(int32_t adc_T);
    uint32_t compensatePressure(int32_t adc_P);
    uint32_t compensateHumidity(int32_t adc_H);
    bool readSensorData();

public:
    BME280_Sensor(const std::string& i2c_device = "/dev/i2c-1", uint8_t address = 0x76);
    virtual ~BME280_Sensor();

    virtual bool init() override;
    virtual const meteo::data& getMeteoConditions() override;
    virtual const std::string whoAmI() const override;
};

#endif // BME280_SENSOR_H