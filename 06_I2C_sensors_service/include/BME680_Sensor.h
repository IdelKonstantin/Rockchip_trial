#ifndef BME680_SENSOR_H
#define BME680_SENSOR_H

#include "iMeteoSensor.h"
#include <cstdint>
#include <string>

class BME680_Sensor : public iMeteoSensor {
private:
    // Структура для калибровочных данных BME680 (только температура, давление, влажность)
    struct CalibData {
        // Температура
        uint16_t par_t1;
        int16_t par_t2, par_t3;
        
        // Давление
        uint16_t par_p1;
        int16_t par_p2, par_p3, par_p4, par_p5, par_p6, par_p7, par_p8, par_p9, par_p10;
        
        // Влажность
        uint16_t par_h1, par_h2;
        int8_t par_h3, par_h4, par_h5, par_h6, par_h7;
    };

    // Регистры BME680
    static constexpr uint8_t REGISTER_CHIP_ID       = 0xD0;
    static constexpr uint8_t REGISTER_RESET         = 0xE0;
    static constexpr uint8_t REGISTER_CTRL_MEAS     = 0x74;
    static constexpr uint8_t REGISTER_CTRL_HUM      = 0x72;
    static constexpr uint8_t REGISTER_DATA          = 0x1F;
    static constexpr uint8_t REGISTER_CALIB_DATA    = 0x89;

    static constexpr uint8_t CHIP_ID_BME680         = 0x61;

    int i2c_fd;
    uint8_t i2c_addr;
    CalibData calib;
    meteo::data current_data{15.0f, 1013, 50, 4.0f, 90};
    bool initialized{false};
    std::string i2c_device;
    int32_t t_fine{0};

    // Функции для работы с I2C
    uint8_t readByte(uint8_t reg);
    uint16_t readWord(uint8_t reg);
    int16_t readWordSigned(uint8_t reg);
    int8_t readByteSigned(uint8_t reg);
    void writeByte(uint8_t reg, uint8_t value);
    void readBlock(uint8_t reg, uint8_t* data, uint8_t length);

    // Внутренние методы
    void readCalibrationData();
    void initializeSensor();
    double compensateTemperature(int32_t raw_temp);
    double compensatePressure(int32_t raw_pressure);
    double compensateHumidity(int32_t raw_humidity);
    bool readSensorData();

public:
    BME680_Sensor(const std::string& i2c_device = "/dev/i2c-1", uint8_t address = 0x76);
    virtual ~BME680_Sensor();

    virtual bool init() override;
    virtual const meteo::data& getMeteoConditions() override;
    virtual const std::string whoAmI() const override;
};

#endif // BME680_SENSOR_H