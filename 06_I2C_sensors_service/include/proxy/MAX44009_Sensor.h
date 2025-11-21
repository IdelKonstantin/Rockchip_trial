#ifndef MAX44009_PROXIMITY_SENSOR_H
#define MAX44009_PROXIMITY_SENSOR_H

#include "iProximitySensor.h"
#include <cstdint>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

class MAX44009_Sensor : public iProximitySensor {
private:
    // Регистры MAX44009
    static constexpr uint8_t MAX44009_INT_STATUS   = 0x00;
    static constexpr uint8_t MAX44009_INT_ENABLE   = 0x01;
    static constexpr uint8_t MAX44009_CONFIG       = 0x02;
    static constexpr uint8_t MAX44009_LUX_HIGH     = 0x03;
    static constexpr uint8_t MAX44009_LUX_LOW      = 0x04;
    static constexpr uint8_t MAX44009_THRESH_HIGH  = 0x05;
    static constexpr uint8_t MAX44009_THRESH_LOW   = 0x06;
    static constexpr uint8_t MAX44009_THRESH_TIMER = 0x07;

    // Адреса устройства
    static constexpr uint8_t MAX44009_I2C_ADDR     = 0x4A;

    // Пороги изменения освещенности для обнаружения приближения (%)
    static constexpr float PROXIMITY_THRESHOLD_5CM  = 0.7f;  // 70% падение света
    static constexpr float PROXIMITY_THRESHOLD_10CM = 0.5f;  // 50% падение света  
    static constexpr float PROXIMITY_THRESHOLD_15CM = 0.3f;  // 30% падение света
    static constexpr float PROXIMITY_THRESHOLD_20CM = 0.2f;  // 20% падение света

    int i2c_fd;
    uint8_t i2c_addr;
    proxy::data current_data;
    bool initialized;
    std::string i2c_device;
    float proximity_threshold;
    float baseline_lux;
    float current_lux;
    bool baseline_calibrated;

    // Функции для работы с I2C
    bool writeByte(uint8_t reg, uint8_t value);
    uint8_t readByte(uint8_t reg);
    float calculateLux(uint8_t high_byte, uint8_t low_byte);
    void updateProximityStatus();
    bool calibrateBaseline();

public:
    MAX44009_Sensor(const std::string& i2c_device = "/dev/i2c-1", uint8_t address = MAX44009_I2C_ADDR);
    virtual ~MAX44009_Sensor();

    virtual bool init() override;
    virtual const proxy::data& getProximityStatus() override;
    virtual const std::string whoAmI() const override;

    // Метод для установки дистанции срабатывания
    void setDetectionDistance(int distance_cm);
    
    // Дополнительные методы
    float getCurrentLux();
    float getBaselineLux();
    void recalibrateBaseline();
    bool isBaselineCalibrated() const;

private:
    bool configureSensor();
};

#endif // MAX44009_PROXIMITY_SENSOR_H