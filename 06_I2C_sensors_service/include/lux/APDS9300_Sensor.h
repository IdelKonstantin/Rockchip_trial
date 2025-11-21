#ifndef APDS9300_SENSOR_H
#define APDS9300_SENSOR_H

#include "iLightSensor.h"
#include <cstdint>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

class APDS9300_Sensor : public iLightSensor {
private:
    // Регистры APDS-9300
    static constexpr uint8_t APDS9300_CONTROL       = 0x00;
    static constexpr uint8_t APDS9300_TIMING        = 0x01;
    static constexpr uint8_t APDS9300_THRESHLOWLOW  = 0x02;
    static constexpr uint8_t APDS9300_THRESHLOWHIGH = 0x03;
    static constexpr uint8_t APDS9300_THRESHHIGHLOW = 0x04;
    static constexpr uint8_t APDS9300_THRESHHIGHHIGH = 0x05;
    static constexpr uint8_t APDS9300_INTERRUPT     = 0x06;
    static constexpr uint8_t APDS9300_ID            = 0x0A;
    static constexpr uint8_t APDS9300_DATA0LOW      = 0x0C;
    static constexpr uint8_t APDS9300_DATA0HIGH     = 0x0D;
    static constexpr uint8_t APDS9300_DATA1LOW      = 0x0E;
    static constexpr uint8_t APDS9300_DATA1HIGH     = 0x0F;

    // Команды управления
    static constexpr uint8_t APDS9300_POWER_ON      = 0x03;
    static constexpr uint8_t APDS9300_POWER_OFF     = 0x00;

    // Время интеграции
    static constexpr uint8_t APDS9300_INTEGRATIONTIME_13MS  = 0x00;
    static constexpr uint8_t APDS9300_INTEGRATIONTIME_101MS = 0x01;
    static constexpr uint8_t APDS9300_INTEGRATIONTIME_402MS = 0x02;

    // Усиление
    static constexpr uint8_t APDS9300_GAIN_1X       = 0x00;
    static constexpr uint8_t APDS9300_GAIN_16X      = 0x10;

    // ID датчика
    static constexpr uint8_t APDS9300_ID_VALUE      = 0x50;

    // Адреса устройства
    static constexpr uint8_t APDS9300_I2C_ADDR      = 0x39;

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
    uint16_t readWord(uint8_t reg_low);
    bool enable();
    bool disable();
    light::lux_t calculateLux(uint16_t ch0, uint16_t ch1);
    light::level calculateLightLevel(light::lux_t lux);

public:
    APDS9300_Sensor(const std::string& i2c_device = "/dev/i2c-1", 
                   uint8_t address = APDS9300_I2C_ADDR,
                   uint8_t integration = APDS9300_INTEGRATIONTIME_402MS,
                   uint8_t gain_setting = APDS9300_GAIN_1X);
    virtual ~APDS9300_Sensor();

    virtual bool init() override;
    virtual const light::data& getLightData() override;
    virtual const std::string whoAmI() const override;

    // Дополнительные методы для настройки
    void setIntegrationTime(uint8_t time);
    void setGain(uint8_t gain_setting);
    uint8_t getDeviceID();
};

#endif // APDS9300_SENSOR_H