#ifndef MPU6050_SENSOR_H
#define MPU6050_SENSOR_H

#include "iIMUSensor.h"
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"

class MPU6050_Sensor : public iIMUSensor {
private:
    MPU6050 mpu;
    std::string i2c_device;
    
    bool dmp_ready;
    uint16_t packet_size;
    uint16_t fifo_count;
    uint8_t fifo_buffer[64];
    IMU::data current_data;
    
    // DMP переменные
    Quaternion q;
    VectorInt16 aa;         // Акселерометр
    VectorInt16 gg;         // Гироскоп
    VectorFloat gravity;    // Гравитация
    float ypr[3];           // [yaw, pitch, roll]
    
    // Константы преобразования
    static constexpr float RAD_TO_DEG = 57.2957795131f;
    static constexpr float ACCEL_SCALE = 16384.0f;  // для ±2g
    static constexpr float GYRO_SCALE = 131.0f;     // для ±250°/s
    
    bool initializeDMP();
    void updateIMUData();

public:
    MPU6050_Sensor(const std::string& i2c_device = "/dev/i2c-1", uint8_t address = 0x68);
    virtual ~MPU6050_Sensor() = default;

    virtual bool init() override;
    virtual const IMU::data& getIMUData() override;
    virtual const std::string whoAmI() const override;
    
    // Дополнительные методы
    bool testConnection();
    void calibrate();
    void setSensorOffsets(int16_t ax, int16_t ay, int16_t az, int16_t gx, int16_t gy, int16_t gz);
    void getSensorOffsets(int16_t &ax, int16_t &ay, int16_t &az, int16_t &gx, int16_t &gy, int16_t &gz);
    float getTemperature();
    
    // Метод для смены I2C устройства
    void setI2CDevice(const std::string& device);
};

#endif // MPU6050_SENSOR_H