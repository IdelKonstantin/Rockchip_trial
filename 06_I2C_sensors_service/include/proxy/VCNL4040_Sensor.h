#ifndef VCNL4040_SENSOR_H
#define VCNL4040_SENSOR_H

#include "iProximitySensor.h"
#include <cstdint>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

class VCNL4040_Sensor : public iProximitySensor {
private:
    // Регистры VCNL4040
    static constexpr uint8_t VCNL4040_PS_CONF1      = 0x03;
    static constexpr uint8_t VCNL4040_PS_CONF2      = 0x03;
    static constexpr uint8_t VCNL4040_PS_CONF3      = 0x04;
    static constexpr uint8_t VCNL4040_PS_MS         = 0x04;
    static constexpr uint8_t VCNL4040_PS_DATA_L     = 0x08;
    static constexpr uint8_t VCNL4040_PS_DATA_H     = 0x09;
    static constexpr uint8_t VCNL4040_PS_THDL_L     = 0x06;
    static constexpr uint8_t VCNL4040_PS_THDL_H     = 0x07;
    static constexpr uint8_t VCNL4040_PS_THDH_L     = 0x08;
    static constexpr uint8_t VCNL4040_PS_THDH_H     = 0x09;
    static constexpr uint8_t VCNL4040_ID_L          = 0x0C;
    static constexpr uint8_t VCNL4040_ID_H          = 0x0D;

    // Команды и настройки
    static constexpr uint8_t VCNL4040_PSD_START     = 0x01;
    static constexpr uint8_t VCNL4040_PSD_SHUTDOWN  = 0x00;
    
    // Значения ID датчика
    static constexpr uint16_t VCNL4040_DEVICE_ID    = 0x0860;

    // Адреса устройства
    static constexpr uint8_t VCNL4040_I2C_ADDR      = 0x60;

    // Пороги приближения (эмпирические значения для VCNL4040)
    static constexpr uint16_t PROXIMITY_THRESHOLD_5CM   = 200;
    static constexpr uint16_t PROXIMITY_THRESHOLD_10CM  = 100;
    static constexpr uint16_t PROXIMITY_THRESHOLD_15CM  = 50;
    static constexpr uint16_t PROXIMITY_THRESHOLD_20CM  = 20;

    int i2c_fd;
    uint8_t i2c_addr;
    proxy::data current_data;
    bool initialized;
    std::string i2c_device;
    uint16_t proximity_threshold;
    uint16_t current_proximity;

    // Функции для работы с I2C
    bool writeByte(uint8_t reg, uint8_t value);
    bool writeWord(uint8_t reg, uint16_t value);
    uint8_t readByte(uint8_t reg);
    uint16_t readWord(uint8_t reg_low);
    bool setupProximitySensor();
    void updateProximityStatus();

public:
    VCNL4040_Sensor(const std::string& i2c_device = "/dev/i2c-1", 
                   uint8_t address = VCNL4040_I2C_ADDR);
    virtual ~VCNL4040_Sensor();

    virtual bool init() override;
    virtual const proxy::data& getProximityStatus() override;
    virtual const std::string whoAmI() const override;

    // Метод для установки дистанции срабатывания
    void setDetectionDistance(int distance_cm);
    
    // Дополнительные методы
    uint16_t getRawProximity();
    bool enableProximitySensor(bool enable = true);
    uint16_t getAmbientLight();

private:
    bool setProximityIntegrationTime(uint8_t time);
    bool setProximityLEDCurrent(uint8_t current);
};

#endif // VCNL4040_SENSOR_H