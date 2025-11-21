#ifndef APDS9930_SENSOR_H
#define APDS9930_SENSOR_H

#include "iLightSensor.h"
#include <cstdint>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

class APDS9930_Sensor : public iLightSensor {
private:
    // Регистры APDS-9930
    static constexpr uint8_t APDS9930_ENABLE       = 0x00;
    static constexpr uint8_t APDS9930_ATIME        = 0x01;
    static constexpr uint8_t APDS9930_WTIME        = 0x03;
    static constexpr uint8_t APDS9930_AILTL        = 0x04;
    static constexpr uint8_t APDS9930_AILTH        = 0x05;
    static constexpr uint8_t APDS9930_AIHTL        = 0x06;
    static constexpr uint8_t APDS9930_AIHTH        = 0x07;
    static constexpr uint8_t APDS9930_PILTL        = 0x08;
    static constexpr uint8_t APDS9930_PILTH        = 0x09;
    static constexpr uint8_t APDS9930_PIHTL        = 0x0A;
    static constexpr uint8_t APDS9930_PIHTH        = 0x0B;
    static constexpr uint8_t APDS9930_PERS         = 0x0C;
    static constexpr uint8_t APDS9930_CONFIG       = 0x0D;
    static constexpr uint8_t APDS9930_PPULSE       = 0x0E;
    static constexpr uint8_t APDS9930_CONTROL      = 0x0F;
    static constexpr uint8_t APDS9930_ID           = 0x12;
    static constexpr uint8_t APDS9930_STATUS       = 0x13;
    static constexpr uint8_t APDS9930_CDATAL       = 0x14;
    static constexpr uint8_t APDS9930_CDATAH       = 0x15;
    static constexpr uint8_t APDS9930_IRDATAL      = 0x16;
    static constexpr uint8_t APDS9930_IRDATAH      = 0x17;
    static constexpr uint8_t APDS9930_PDATAL       = 0x18;
    static constexpr uint8_t APDS9930_PDATAH       = 0x19;

    // Биты регистра ENABLE
    static constexpr uint8_t APDS9930_PON          = 0x01;
    static constexpr uint8_t APDS9930_AEN          = 0x02;
    static constexpr uint8_t APDS9930_PEN          = 0x04;
    static constexpr uint8_t APDS9930_WEN          = 0x08;
    static constexpr uint8_t APDS9930_AIEN         = 0x10;
    static constexpr uint8_t APDS9930_PIEN         = 0x20;

    // Значения по умолчанию
    static constexpr uint8_t APDS9930_DEFAULT_ATIME = 0xFF; // 2.72ms
    static constexpr uint8_t APDS9930_DEFAULT_WTIME = 0xFF; // 2.72ms
    static constexpr uint8_t APDS9930_DEFAULT_PPULSE = 0x08; // 8 pulses
    static constexpr uint8_t APDS9930_DEFAULT_PERS = 0x11; // 1 proximity, 1 ALS

    // ID датчика
    static constexpr uint8_t APDS9930_ID_VALUE     = 0x39;

    // Адреса устройства
    static constexpr uint8_t APDS9930_I2C_ADDR     = 0x39;

    // Пороги освещенности для определения уровня (в люксах)
    static constexpr light::lux_t THRESHOLD_TOTAL_DARKNESS = 1.0f;
    static constexpr light::lux_t THRESHOLD_VERY_DARK = 10.0f;
    static constexpr light::lux_t THRESHOLD_DARK = 50.0f;
    static constexpr light::lux_t THRESHOLD_TWILIGHT = 200.0f;
    static constexpr light::lux_t THRESHOLD_DIM_LIGHT = 500.0f;
    static constexpr light::lux_t THRESHOLD_NORMAL_LIGHT = 1000.0f;
    static constexpr light::lux_t THRESHOLD_BRIGHT_LIGHT = 10000.0f;
    static constexpr light::lux_t THRESHOLD_VERY_BRIGHT = 40000.0f;

    int i2c_fd;
    uint8_t i2c_addr;
    light::data current_data;
    bool initialized;
    std::string i2c_device;
    float ch0_calibration;
    float ch1_calibration;

    // Функции для работы с I2C
    bool writeByte(uint8_t reg, uint8_t value);
    uint8_t readByte(uint8_t reg);
    uint16_t readWord(uint8_t reg_low);
    bool setMode(uint8_t mode, bool enable);
    light::lux_t calculateLux(uint16_t ch0, uint16_t ch1);
    light::level calculateLightLevel(light::lux_t lux);
    bool setupLightSensor();

public:
    APDS9930_Sensor(const std::string& i2c_device = "/dev/i2c-1", uint8_t address = APDS9930_I2C_ADDR);
    virtual ~APDS9930_Sensor();

    virtual bool init() override;
    virtual const light::data& getLightData() override;
    virtual const std::string whoAmI() const override;

    // Дополнительные методы для работы с приближением
    uint16_t getProximity();
    bool enableProximity(bool enable = true);
    bool enableLightSensor(bool enable = true);
};

#endif // APDS9930_SENSOR_H