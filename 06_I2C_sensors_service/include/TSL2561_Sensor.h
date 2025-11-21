#ifndef TSL2561_SENSOR_H
#define TSL2561_SENSOR_H

#include "iLightSensor.h"
#include <cstdint>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

class TSL2561_Sensor : public iLightSensor {
private:
    // Регистры TSL2561
    static constexpr uint8_t TSL2561_CMD            = 0x80;
    static constexpr uint8_t TSL2561_CMD_CLEAR      = 0xC0;
    static constexpr uint8_t TSL2561_CONTROL        = 0x00;
    static constexpr uint8_t TSL2561_TIMING         = 0x01;
    static constexpr uint8_t TSL2561_THRESHLOWLOW   = 0x02;
    static constexpr uint8_t TSL2561_THRESHLOWHIGH  = 0x03;
    static constexpr uint8_t TSL2561_THRESHHIGHLOW  = 0x04;
    static constexpr uint8_t TSL2561_THRESHHIGHHIGH = 0x05;
    static constexpr uint8_t TSL2561_INTERRUPT      = 0x06;
    static constexpr uint8_t TSL2561_CRC            = 0x08;
    static constexpr uint8_t TSL2561_ID             = 0x0A;
    static constexpr uint8_t TSL2561_DATA0LOW       = 0x0C;
    static constexpr uint8_t TSL2561_DATA0HIGH      = 0x0D;
    static constexpr uint8_t TSL2561_DATA1LOW       = 0x0E;
    static constexpr uint8_t TSL2561_DATA1HIGH      = 0x0F;

    // Команды управления
    static constexpr uint8_t TSL2561_POWER_ON       = 0x03;
    static constexpr uint8_t TSL2561_POWER_OFF      = 0x00;

    // Время интеграции
    static constexpr uint8_t TSL2561_INTEGRATIONTIME_13MS  = 0x00;
    static constexpr uint8_t TSL2561_INTEGRATIONTIME_101MS = 0x01;
    static constexpr uint8_t TSL2561_INTEGRATIONTIME_402MS = 0x02;

    // Усиление
    static constexpr uint8_t TSL2561_GAIN_1X        = 0x00;
    static constexpr uint8_t TSL2561_GAIN_16X       = 0x10;

    // Адреса устройства
    static constexpr uint8_t TSL2561_ADDR_LOW       = 0x29;
    static constexpr uint8_t TSL2561_ADDR_FLOAT     = 0x39;
    static constexpr uint8_t TSL2561_ADDR_HIGH      = 0x49;

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
    uint8_t integration_time;
    uint8_t gain;
    light::data current_data;
    bool initialized;
    std::string i2c_device;

    // Функции для работы с I2C
    bool writeByte(uint8_t reg, uint8_t value);
    uint8_t readByte(uint8_t reg);
    uint16_t readWord(uint8_t reg);
    bool enable();
    bool disable();
    light::lux_t calculateLux(uint16_t ch0, uint16_t ch1);
    light::level calculateLightLevel(light::lux_t lux);

public:
    TSL2561_Sensor(const std::string& i2c_device = "/dev/i2c-1", 
                   uint8_t address = TSL2561_ADDR_FLOAT,
                   uint8_t integration = TSL2561_INTEGRATIONTIME_402MS,
                   uint8_t gain_setting = TSL2561_GAIN_1X);
    virtual ~TSL2561_Sensor();

    virtual bool init() override;
    virtual const light::data& getLightData() override;
    virtual const std::string whoAmI() const override;

    // Дополнительные методы для настройки
    void setIntegrationTime(uint8_t time);
    void setGain(uint8_t gain_setting);
};

#endif // TSL2561_SENSOR_H