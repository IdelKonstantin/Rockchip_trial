#ifndef _ICM20948_SENSOR_H_
#define _ICM20948_SENSOR_H_

#include "iIMUSensor.h"
#include <string>

class ICM20948_Sensor : public iIMUSensor
{
public:
    ICM20948_Sensor(const std::string& i2c_device = "/dev/i2c-1", uint8_t address = 0x68);
    virtual ~ICM20948_Sensor();

    const IMU::data& getIMUData() override;
    bool init() override;
    const std::string whoAmI() const override;

private:
    class ICM20948_Impl;
    ICM20948_Impl* mImpl;
    IMU::data mData;
    bool mInitialized;
    std::string mI2CDevice;
    uint8_t mDeviceAddress;
};

#endif /* _ICM20948_SENSOR_H_ */