#include "MPU6050_Sensor.h"

#include <cmath>
#include <unistd.h>

MPU6050_Sensor::MPU6050_Sensor(const std::string& i2c_device, uint8_t address) 
    : mpu(address), i2c_device(i2c_device), dmp_ready(false), packet_size(0) {
    
    // Инициализация структуры данных
    current_data = {
        {0.0f, 0.0f, 0.0f},  // acceleration
        {0.0f, 0.0f, 0.0f},  // gyroscope  
        {0.0f, 0.0f, 0.0f}   // angle
    };
}

bool MPU6050_Sensor::initializeDMP() {
    // Сброс и инициализация DMP
    mpu.reset();
    usleep(100000); // 100ms задержка
    
    uint8_t dev_status = mpu.dmpInitialize();
    
    // Установка калибровочных смещений (примерные значения)
    mpu.setXGyroOffset(51);
    mpu.setYGyroOffset(8);
    mpu.setZGyroOffset(21);
    mpu.setXAccelOffset(1150);
    mpu.setYAccelOffset(-50);
    mpu.setZAccelOffset(1060);
    
    if (dev_status == 0) {
        // Включение DMP
        mpu.setDMPEnabled(true);
        
        // Получение размера пакета DMP
        packet_size = mpu.dmpGetFIFOPacketSize();
        dmp_ready = true;
        
        return true;
    } else {
        return false;
    }
}

void MPU6050_Sensor::updateIMUData() {
    if (!dmp_ready) return;
    
    // Получение количества данных в FIFO
    fifo_count = mpu.getFIFOCount();
    
    if (fifo_count >= packet_size) {
        // Проверяем, что в FIFO достаточно данных для полного пакета
        if (fifo_count % packet_size != 0) {
            // Сбрасываем FIFO если данные не выровнены по пакетам
            mpu.resetFIFO();
            return;
        }
        
        // Чтение пакета из FIFO
        mpu.getFIFOBytes(fifo_buffer, packet_size);
        
        // Проверяем валидность пакета (первый байт должен быть 0x00 или 0x01)
        if (fifo_buffer[0] & 0x02) {
            // Пакет невалиден, сбрасываем FIFO
            mpu.resetFIFO();
            return;
        }
        
        // Получение данных DMP
        mpu.dmpGetQuaternion(&q, fifo_buffer);
        mpu.dmpGetAccel(&aa, fifo_buffer);
        mpu.dmpGetGyro(&gg, fifo_buffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
        
        // Обновление структуры данных согласно интерфейсу
        // Акселерометр (g)
        current_data.acceleration.x = aa.x / ACCEL_SCALE;
        current_data.acceleration.y = aa.y / ACCEL_SCALE;
        current_data.acceleration.z = aa.z / ACCEL_SCALE;
        
        // Гироскоп (град/с)
        current_data.gyroscope.x = gg.x / GYRO_SCALE;
        current_data.gyroscope.y = gg.y / GYRO_SCALE;
        current_data.gyroscope.z = gg.z / GYRO_SCALE;
        
        // Углы (градусы)
        // DMP возвращает [yaw, pitch, roll], преобразуем к интерфейсу [roll, pitch, yaw]
        current_data.angle.roll = ypr[2] * RAD_TO_DEG;   // Roll
        current_data.angle.pitch = ypr[1] * RAD_TO_DEG;  // Pitch
        current_data.angle.yaw = ypr[0] * RAD_TO_DEG;    // Yaw
    }
    
    // Сброс FIFO если переполнение
    if (fifo_count >= 1024) {
        mpu.resetFIFO();
    }
}

bool MPU6050_Sensor::init() {
    // Инициализация I2C с указанием устройства
    I2Cdev::initialize(i2c_device.c_str());
    
    // Инициализация MPU6050
    mpu.initialize();
    
    // Проверка соединения
    if (!testConnection()) {
        return false;
    }
    
    // Проверка WhoAmI
    uint8_t device_id = mpu.getDeviceID();
    
    if (device_id != 0x34 && device_id != 0x68 && device_id != 0x70 && device_id != 0x72) {
        return false;
    }
    
    // Настройка диапазонов
    mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);
    mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
    
    // Настройка DLPF (Digital Low Pass Filter)
    mpu.setDLPFMode(MPU6050_DLPF_BW_42);
    
    // Частота дискретизации
    mpu.setRate(4); // 1kHz / (1 + 4) = 200Hz
    
    // Инициализация DMP
    if (!initializeDMP()) {
        return false;
    }
    
    return true;
}

const IMU::data& MPU6050_Sensor::getIMUData() {
    updateIMUData();
    return current_data;
}

const std::string MPU6050_Sensor::whoAmI() const {
    return "MPU6050";
}

bool MPU6050_Sensor::testConnection() {
    return mpu.testConnection();
}

void MPU6050_Sensor::calibrate() {
    // Калибровка акселерометра (6 итераций)
    mpu.CalibrateAccel(6);
    
    // Калибровка гироскопа (6 итераций)
    mpu.CalibrateGyro(6);
    
    // Сохранение калибровочных значений
    mpu.PrintActiveOffsets();
}

void MPU6050_Sensor::setSensorOffsets(int16_t ax, int16_t ay, int16_t az, int16_t gx, int16_t gy, int16_t gz) {
    mpu.setXAccelOffset(ax);
    mpu.setYAccelOffset(ay);
    mpu.setZAccelOffset(az);
    mpu.setXGyroOffset(gx);
    mpu.setYGyroOffset(gy);
    mpu.setZGyroOffset(gz);
}

void MPU6050_Sensor::getSensorOffsets(int16_t &ax, int16_t &ay, int16_t &az, int16_t &gx, int16_t &gy, int16_t &gz) {
    ax = mpu.getXAccelOffset();
    ay = mpu.getYAccelOffset();
    az = mpu.getZAccelOffset();
    gx = mpu.getXGyroOffset();
    gy = mpu.getYGyroOffset();
    gz = mpu.getZGyroOffset();
}

float MPU6050_Sensor::getTemperature() {
    int16_t temp = mpu.getTemperature();
    return (temp / 340.0f) + 36.53f;
}

void MPU6050_Sensor::setI2CDevice(const std::string& device) {
    i2c_device = device;
    // При смене устройства нужно переинициализировать сенсор
    if (dmp_ready) {
        dmp_ready = false;
    }
}