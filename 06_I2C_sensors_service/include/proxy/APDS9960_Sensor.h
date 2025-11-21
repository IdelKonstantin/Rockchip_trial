#ifndef APDS9960_SENSOR_H
#define APDS9960_SENSOR_H

#include "iProximitySensor.h"
#include <cstdint>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

class APDS9960_Sensor : public iProximitySensor {
private:
    // Регистры APDS-9960
    static constexpr uint8_t APDS9960_ENABLE       = 0x80;
    static constexpr uint8_t APDS9960_ATIME        = 0x81;
    static constexpr uint8_t APDS9960_WTIME        = 0x83;
    static constexpr uint8_t APDS9960_AILTL        = 0x84;
    static constexpr uint8_t APDS9960_AILTH        = 0x85;
    static constexpr uint8_t APDS9960_AIHTL        = 0x86;
    static constexpr uint8_t APDS9960_AIHTH        = 0x87;
    static constexpr uint8_t APDS9960_PILTL        = 0x89;
    static constexpr uint8_t APDS9960_PILTH        = 0x8A;
    static constexpr uint8_t APDS9960_PIHTL        = 0x8B;
    static constexpr uint8_t APDS9960_PIHTH        = 0x8C;
    static constexpr uint8_t APDS9960_PERS         = 0x8C;
    static constexpr uint8_t APDS9960_CONFIG1      = 0x8D;
    static constexpr uint8_t APDS9960_PPULSE       = 0x8E;
    static constexpr uint8_t APDS9960_CONTROL      = 0x8F;
    static constexpr uint8_t APDS9960_CONFIG2      = 0x90;
    static constexpr uint8_t APDS9960_ID           = 0x92;
    static constexpr uint8_t APDS9960_STATUS       = 0x93;
    static constexpr uint8_t APDS9960_CDATAL       = 0x94;
    static constexpr uint8_t APDS9960_CDATAH       = 0x95;
    static constexpr uint8_t APDS9960_RDATAL       = 0x96;
    static constexpr uint8_t APDS9960_RDATAH       = 0x97;
    static constexpr uint8_t APDS9960_GDATAL       = 0x98;
    static constexpr uint8_t APDS9960_GDATAH       = 0x99;
    static constexpr uint8_t APDS9960_BDATAL       = 0x9A;
    static constexpr uint8_t APDS9960_BDATAH       = 0x9B;
    static constexpr uint8_t APDS9960_PDATA        = 0x9C;
    static constexpr uint8_t APDS9960_POFFSET_UR   = 0x9D;
    static constexpr uint8_t APDS9960_POFFSET_DL   = 0x9E;
    static constexpr uint8_t APDS9960_CONFIG3      = 0x9F;
    static constexpr uint8_t APDS9960_GPENTH       = 0xA0;
    static constexpr uint8_t APDS9960_GEXTH        = 0xA1;
    static constexpr uint8_t APDS9960_GCONF1       = 0xA2;
    static constexpr uint8_t APDS9960_GCONF2       = 0xA3;
    static constexpr uint8_t APDS9960_GOFFSET_U    = 0xA4;
    static constexpr uint8_t APDS9960_GOFFSET_D    = 0xA5;
    static constexpr uint8_t APDS9960_GOFFSET_L    = 0xA7;
    static constexpr uint8_t APDS9960_GOFFSET_R    = 0xA9;
    static constexpr uint8_t APDS9960_GPULSE       = 0xA6;
    static constexpr uint8_t APDS9960_GCONF3       = 0xAA;
    static constexpr uint8_t APDS9960_GCONF4       = 0xAB;
    static constexpr uint8_t APDS9960_GFLVL        = 0xAE;
    static constexpr uint8_t APDS9960_GSTATUS      = 0xAF;

    // Биты регистра ENABLE
    static constexpr uint8_t APDS9960_PON          = 0x01;
    static constexpr uint8_t APDS9960_AEN          = 0x02;
    static constexpr uint8_t APDS9960_PEN          = 0x04;
    static constexpr uint8_t APDS9960_WEN          = 0x08;
    static constexpr uint8_t APDS9960_AIEN         = 0x10;
    static constexpr uint8_t APDS9960_PIEN         = 0x20;
    static constexpr uint8_t APDS9960_GEN          = 0x40;

    // ID датчика
    static constexpr uint8_t APDS9960_ID_VALUE     = 0xAB;

    // Адреса устройства
    static constexpr uint8_t APDS9960_I2C_ADDR     = 0x39;

    // Пороги приближения (эмпирические значения)
    static constexpr uint8_t PROXIMITY_THRESHOLD_5CM  = 50;
    static constexpr uint8_t PROXIMITY_THRESHOLD_10CM = 30;
    static constexpr uint8_t PROXIMITY_THRESHOLD_15CM = 20;
    static constexpr uint8_t PROXIMITY_THRESHOLD_20CM = 10;

    int i2c_fd;
    uint8_t i2c_addr;
    proxy::data current_data;
    bool initialized;
    std::string i2c_device;
    uint8_t proximity_threshold;
    uint8_t current_proximity;

    // Функции для работы с I2C
    bool writeByte(uint8_t reg, uint8_t value);
    uint8_t readByte(uint8_t reg);
    bool setMode(uint8_t mode, bool enable);
    bool setupProximitySensor();
    void updateProximityStatus();

public:
    APDS9960_Sensor(const std::string& i2c_device = "/dev/i2c-1", 
                   uint8_t address = APDS9960_I2C_ADDR);
    virtual ~APDS9960_Sensor();

    virtual bool init() override;
    virtual const proxy::data& getProximityStatus() override;
    virtual const std::string whoAmI() const override;

    // Метод для установки дистанции срабатывания
    void setDetectionDistance(int distance_cm);
    
    // Дополнительные методы
    uint8_t getRawProximity();
    bool enableGestureSensor(bool enable = true);
    bool enableLightSensor(bool enable = true);
};

#endif // APDS9960_SENSOR_H