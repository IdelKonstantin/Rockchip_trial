#ifndef BH1750_SENSOR_H
#define BH1750_SENSOR_H

#include "iLightSensor.h"
#include <cstdint>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

class BH1750_Sensor : public iLightSensor {
private:
    // Регистры и команды BH1750
    static constexpr uint8_t BH1750_POWER_DOWN     = 0x00;
    static constexpr uint8_t BH1750_POWER_ON       = 0x01;
    static constexpr uint8_t BH1750_RESET          = 0x07;
    static constexpr uint8_t BH1750_CONTINUOUS_HIGH_RES_MODE  = 0x10;
    static constexpr uint8_t BH1750_CONTINUOUS_HIGH_RES_MODE2 = 0x11;
    static constexpr uint8_t BH1750_CONTINUOUS_LOW_RES_MODE   = 0x13;
    static constexpr uint8_t BH1750_ONE_TIME_HIGH_RES_MODE    = 0x20;
    static constexpr uint8_t BH1750_ONE_TIME_HIGH_RES_MODE2   = 0x21;
    static constexpr uint8_t BH1750_ONE_TIME_LOW_RES_MODE     = 0x23;

    static constexpr uint8_t BH1750_DEFAULT_ADDRESS = 0x23;

    // Пороги освещенности для определения уровня (в люксах)
    static constexpr light::lux_t THRESHOLD_TOTAL_DARKNESS = 1.0f;
    static constexpr light::lux_t THRESHOLD_VERY_DARK = 10.0f;
    static constexpr light::lux_t THRESHOLD_DARK = 50.0f;
    static constexpr light::lux_t THRESHOLD_TWILIGHT = 200.0f;
    static constexpr light::lux_t THRESHOLD_DIM_LIGHT = 500.0f;
    static constexpr light::lux_t THRESHOLD_NORMAL_LIGHT = 1000.0f;
    static constexpr light::lux_t THRESHOLD_BRIGHT_LIGHT = 10000.0f;
    // above 10000 = VERY_BRIGHT

    int i2c_fd;
    uint8_t i2c_addr;
    light::data current_data;
    bool initialized;
    std::string i2c_device;

    // Функции для работы с I2C
    bool writeCommand(uint8_t command);
    bool readData(uint16_t &data);
    light::level calculateLightLevel(light::lux_t lux);

public:
    BH1750_Sensor(const std::string& i2c_device = "/dev/i2c-1", uint8_t address = BH1750_DEFAULT_ADDRESS);
    virtual ~BH1750_Sensor();

    virtual bool init() override;
    virtual const light::data& getLightData() override;
    virtual const std::string whoAmI() const override;
};

#endif // BH1750_SENSOR_H