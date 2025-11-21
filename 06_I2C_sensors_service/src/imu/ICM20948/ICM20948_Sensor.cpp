#include "ICM20948_Sensor.h"
#include "I2C.h"
#include "imu_data.h"
#include "madgwick_ahrs.h"
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>

// Constants from original icm20948.cpp
#define I2C_ADD_ICM20948_AK09916    0x0C
#define I2C_ADD_ICM20948_AK09916_READ  0x80
#define I2C_ADD_ICM20948_AK09916_WRITE 0x00

/* define ICM-20948 Register */
#define REG_ADD_WIA             0x00
#define REG_VAL_WIA             0xEA
#define REG_ADD_USER_CTRL       0x03
#define REG_VAL_BIT_I2C_MST_EN  0x20
#define REG_ADD_PWR_MGMT_1      0x06
#define REG_VAL_ALL_RGE_RESET   0x80
#define REG_VAL_RUN_MODE        0x01
#define REG_ADD_PWR_MGMT_2      0x07
#define REG_VAL_SENSORS_ON      0x00
#define REG_VAL_DISABLE_GYRO    0x07
#define REG_VAL_DISABLE_ACC     0x38
#define REG_ADD_ACCEL_XOUT_H    0x2D
#define REG_ADD_GYRO_XOUT_H     0x33
#define REG_ADD_TEMP_OUT_H      0x39
#define REG_ADD_EXT_SENS_DATA_00 0x3B
#define REG_ADD_REG_BANK_SEL    0x7F
#define REG_VAL_REG_BANK_0      0x00
#define REG_VAL_REG_BANK_1      0x10
#define REG_VAL_REG_BANK_2      0x20
#define REG_VAL_REG_BANK_3      0x30

/* user bank 2 register */
#define REG_ADD_GYRO_SMPLRT_DIV 0x00
#define REG_ADD_GYRO_CONFIG_1   0x01
#define REG_VAL_BIT_GYRO_FS_250DPS  0x00
#define REG_VAL_BIT_GYRO_FS_500DPS  0x02
#define REG_VAL_BIT_GYRO_FS_1000DPS 0x04
#define REG_VAL_BIT_GYRO_FS_2000DPS 0x06
#define REG_VAL_BIT_GYRO_DLPF       0x01

// Gyro offset registers
#define REG_ADD_XG_OFFS_USRH    0x03
#define REG_ADD_XG_OFFS_USRL    0x04
#define REG_ADD_YG_OFFS_USRH    0x05
#define REG_ADD_YG_OFFS_USRL    0x06
#define REG_ADD_ZG_OFFS_USRH    0x07
#define REG_ADD_ZG_OFFS_USRL    0x08

#define REG_ADD_ACCEL_CONFIG    0x14
#define REG_VAL_BIT_ACCEL_FS_2g     0x00
#define REG_VAL_BIT_ACCEL_FS_4g     0x02
#define REG_VAL_BIT_ACCEL_FS_8g     0x04
#define REG_VAL_BIT_ACCEL_FS_16g    0x06
#define REG_VAL_BIT_ACCEL_DLPF      0x01

/* user bank 3 register */
#define REG_ADD_I2C_MST_CTRL    0x01
#define REG_VAL_I2C_MST_CTRL_CLK_400KHZ 0x07
#define REG_ADD_I2C_SLV0_ADDR   0x03
#define REG_ADD_I2C_SLV0_REG    0x04
#define REG_ADD_I2C_SLV0_CTRL   0x05
#define REG_ADD_I2C_SLV0_DO     0x06
#define REG_VAL_BIT_SLV0_EN     0x80
#define REG_VAL_BIT_MASK_LEN    0x07

/* define ICM-20948 MAG Register */
#define REG_ADD_MAG_WIA1        0x00
#define REG_VAL_MAG_WIA1        0x48
#define REG_ADD_MAG_WIA2        0x01
#define REG_VAL_MAG_WIA2        0x09
#define REG_ADD_MAG_ST1         0x10
#define REG_ADD_MAG_DATA        0x11
#define REG_ADD_MAG_CNTL2       0x31
#define REG_VAL_MAG_MODE_100HZ  0x08

namespace {
    const short IMU_DATA_LEN = 22;
    const short GYRO_AND_ACC_DATA_LEN = 6;
    const short MAG_DATA_LEN = 8;
    const float MAG_SCALE = 0.15f;
    const float DEG_TO_RAD = M_PI / 180.0f;
    
    void sleepMS(const short milliseconds)
    {
        usleep(milliseconds * 1000);
    }
}

class ICM20948_Sensor::ICM20948_Impl
{
public:
    enum ICM_BANK
    {
        BANK_0 = 0x00,
        BANK_1 = 0x10,
        BANK_2 = 0x20,
        BANK_3 = 0x30,
        BANK_UNDEFINED = 0xFF
    };

    ICM20948_Impl(const std::string& i2c_device, uint8_t address) 
        : mI2CDevice(i2c_device), mDeviceAddress(address), 
          mCurrentBank(BANK_UNDEFINED), mGyroScale(0.0f), mAccScale(0.0f) 
    {
        // Initialize quaternion to identity
        mData.mQuat[0] = 1.0f;
        mData.mQuat[1] = 0.0f;
        mData.mQuat[2] = 0.0f;
        mData.mQuat[3] = 0.0f;
    }
    
    bool initialize()
    {
        if (!mI2C.openSerialPort(mI2CDevice.c_str()))
        {
            return false;
        }

        setBank(BANK_0);
        uint8_t deviceID = mI2C.readByte(mDeviceAddress, REG_ADD_WIA);
        if (REG_VAL_WIA != deviceID)
        {
            return false;
        }

        reset();
        configureMasterI2C();
        configureGyro();
        configureAcc();
        bool magOk = configureMag();

        // Если магнитометр не найден, продолжаем без него
        if (!magOk) {
            // Можно добавить логгирование
        }

        return true;
    }

    const IMUData& getData()
    {
        int16_t s16Gyro[3], s16Accel[3], s16Magn[3];
        int16_t temperature;

        bool magDataValid = readAllRawData(s16Gyro, s16Accel, s16Magn, temperature);

        mData.mGyro[0] = static_cast<float>(s16Gyro[0]) * mGyroScale * DEG_TO_RAD;
        mData.mGyro[1] = static_cast<float>(s16Gyro[1]) * mGyroScale * DEG_TO_RAD;
        mData.mGyro[2] = static_cast<float>(s16Gyro[2]) * mGyroScale * DEG_TO_RAD;

        mData.mAcc[0] = static_cast<float>(s16Accel[0]) * mAccScale;
        mData.mAcc[1] = static_cast<float>(s16Accel[1]) * mAccScale;
        mData.mAcc[2] = static_cast<float>(s16Accel[2]) * mAccScale;

        mData.mMag[0] = static_cast<float>(s16Magn[0]) * MAG_SCALE;
        mData.mMag[1] = static_cast<float>(s16Magn[1]) * MAG_SCALE;
        mData.mMag[2] = static_cast<float>(s16Magn[2]) * MAG_SCALE;

        mData.mTemp = (static_cast<float>(temperature - 21) / 333.87f) + 21.0f;
        mData.mUpdatePeriod = 0.01f; // 100Hz

        // Используем версию без магнитометра если данные магнитометра невалидны
        if (magDataValid) {
            MadgwickAHRSupdate(mData);
        } else {
            MadgwickAHRSupdateIMU(mData);
        }

        return mData;
    }

    void calibrateGyro()
    {
        // Сбрасываем калибровку гироскопа
        setBank(BANK_2);
        mI2C.writeByte(mDeviceAddress, REG_ADD_XG_OFFS_USRH, 0);
        mI2C.writeByte(mDeviceAddress, REG_ADD_XG_OFFS_USRL, 0);
        mI2C.writeByte(mDeviceAddress, REG_ADD_YG_OFFS_USRH, 0);
        mI2C.writeByte(mDeviceAddress, REG_ADD_YG_OFFS_USRL, 0);
        mI2C.writeByte(mDeviceAddress, REG_ADD_ZG_OFFS_USRH, 0);
        mI2C.writeByte(mDeviceAddress, REG_ADD_ZG_OFFS_USRL, 0);
        
        // В реальной реализации здесь должна быть полноценная калибровка
        // с усреднением множества измерений
    }

private:
    void setBank(const ICM_BANK bank)
    {
        if (bank != mCurrentBank && bank != BANK_UNDEFINED)
        {
            mI2C.writeByte(mDeviceAddress, REG_ADD_REG_BANK_SEL, bank);
            mCurrentBank = bank;
        }
    }

    void reset()
    {
        setBank(BANK_0);
        mI2C.writeByte(mDeviceAddress, REG_ADD_PWR_MGMT_1, REG_VAL_ALL_RGE_RESET);
        sleepMS(10);
        mI2C.writeByte(mDeviceAddress, REG_ADD_PWR_MGMT_1, REG_VAL_RUN_MODE);
        mI2C.writeByte(mDeviceAddress, REG_ADD_PWR_MGMT_2, REG_VAL_SENSORS_ON);
        sleepMS(10);
    }

    void configureGyro()
    {
        mGyroScale = 250.0f / 32768.0f; // 250DPS default
        
        setBank(BANK_2);
        mI2C.writeByte(mDeviceAddress, REG_ADD_GYRO_SMPLRT_DIV, 4); // 225Hz
        mI2C.writeByte(mDeviceAddress, REG_ADD_GYRO_CONFIG_1, 
                      REG_VAL_BIT_GYRO_FS_250DPS | REG_VAL_BIT_GYRO_DLPF);
    }

    void configureAcc()
    {
        mAccScale = 2.0f / 32768.0f; // 2G default
        
        setBank(BANK_2);
        mI2C.writeByte(mDeviceAddress, REG_ADD_ACCEL_CONFIG, 
                      REG_VAL_BIT_ACCEL_FS_2g | REG_VAL_BIT_ACCEL_DLPF);
    }

    bool configureMag()
    {
        uint8_t u8Data[2];
        magI2CRead(REG_ADD_MAG_WIA1, 2, u8Data);
        
        if ((u8Data[0] != REG_VAL_MAG_WIA1) || (u8Data[1] != REG_VAL_MAG_WIA2))
        {
            return false;
        }

        magI2CWrite(REG_ADD_MAG_CNTL2, REG_VAL_MAG_MODE_100HZ);
        sleepMS(10);
        
        return true;
    }

    void configureMasterI2C()
    {
        setBank(BANK_0);
        uint8_t temp = mI2C.readByte(mDeviceAddress, REG_ADD_USER_CTRL);
        mI2C.writeByte(mDeviceAddress, REG_ADD_USER_CTRL, temp | REG_VAL_BIT_I2C_MST_EN);

        setBank(BANK_3);
        mI2C.writeByte(mDeviceAddress, REG_ADD_I2C_MST_CTRL, REG_VAL_I2C_MST_CTRL_CLK_400KHZ);
        sleepMS(10);
    }

    bool readAllRawData(int16_t gyro[3], int16_t acc[3], int16_t mag[3], int16_t& temp)
    {
        uint8_t u8Buf[IMU_DATA_LEN];
        setBank(BANK_0);
        
        // Метод readNBytes возвращает void, поэтому просто вызываем его
        mI2C.readNBytes(mDeviceAddress, REG_ADD_ACCEL_XOUT_H, IMU_DATA_LEN, u8Buf);

        // Parse accelerometer data
        acc[0] = static_cast<int16_t>((u8Buf[0] << 8) | u8Buf[1]);
        acc[1] = static_cast<int16_t>((u8Buf[2] << 8) | u8Buf[3]);
        acc[2] = static_cast<int16_t>((u8Buf[4] << 8) | u8Buf[5]);

        // Parse gyroscope data
        gyro[0] = static_cast<int16_t>((u8Buf[6] << 8) | u8Buf[7]);
        gyro[1] = static_cast<int16_t>((u8Buf[8] << 8) | u8Buf[9]);
        gyro[2] = static_cast<int16_t>((u8Buf[10] << 8) | u8Buf[11]);

        // Parse temperature data
        temp = static_cast<int16_t>((u8Buf[12] << 8) | u8Buf[13]);

        // Parse magnetic data
        mag[0] = static_cast<int16_t>((u8Buf[15] << 8) | u8Buf[14]);
        mag[1] = -static_cast<int16_t>((u8Buf[17] << 8) | u8Buf[16]);
        mag[2] = -static_cast<int16_t>((u8Buf[19] << 8) | u8Buf[18]);

        // Check magnetometer data ready status (bit 3 of ST2 register)
        return !(u8Buf[21] & 0x08);
    }

    void magI2CRead(const uint8_t regAddr, const uint8_t length, uint8_t* data)
    {
        setBank(BANK_3);
        mI2C.writeByte(mDeviceAddress, REG_ADD_I2C_SLV0_ADDR, 
                      I2C_ADD_ICM20948_AK09916 | I2C_ADD_ICM20948_AK09916_READ);
        mI2C.writeByte(mDeviceAddress, REG_ADD_I2C_SLV0_REG, regAddr);
        mI2C.writeByte(mDeviceAddress, REG_ADD_I2C_SLV0_CTRL, 
                      REG_VAL_BIT_SLV0_EN | length);

        setBank(BANK_0);
        mI2C.readNBytes(mDeviceAddress, REG_ADD_EXT_SENS_DATA_00, length, data);
    }

    void magI2CWrite(const uint8_t regAddr, const uint8_t value)
    {
        setBank(BANK_3);
        mI2C.writeByte(mDeviceAddress, REG_ADD_I2C_SLV0_ADDR, 
                      I2C_ADD_ICM20948_AK09916 | I2C_ADD_ICM20948_AK09916_WRITE);
        mI2C.writeByte(mDeviceAddress, REG_ADD_I2C_SLV0_REG, regAddr);
        mI2C.writeByte(mDeviceAddress, REG_ADD_I2C_SLV0_DO, value);
        mI2C.writeByte(mDeviceAddress, REG_ADD_I2C_SLV0_CTRL, REG_VAL_BIT_SLV0_EN | 1);

        sleepMS(100);
    }

    std::string mI2CDevice;
    uint8_t mDeviceAddress;
    mutable ICM_BANK mCurrentBank;
    float mGyroScale;
    float mAccScale;
    IMUData mData;
    I2C mI2C;
};

// ICM20948_Sensor implementation
ICM20948_Sensor::ICM20948_Sensor(const std::string& i2c_device, uint8_t address) 
    : mImpl(new ICM20948_Impl(i2c_device, address)), 
      mInitialized(false),
      mI2CDevice(i2c_device),
      mDeviceAddress(address)
{
    mData.acceleration = {0.0f, 0.0f, 0.0f};
    mData.gyroscope = {0.0f, 0.0f, 0.0f};
    mData.angle = {0.0f, 0.0f, 0.0f};
}

ICM20948_Sensor::~ICM20948_Sensor()
{
    delete mImpl;
}

const IMU::data& ICM20948_Sensor::getIMUData()
{
    if (!mInitialized)
    {
        return mData;
    }

    const IMUData& rawData = mImpl->getData();

    mData.acceleration.x = rawData.mAcc[0];
    mData.acceleration.y = rawData.mAcc[1];
    mData.acceleration.z = rawData.mAcc[2];

    mData.gyroscope.x = rawData.mGyro[0];
    mData.gyroscope.y = rawData.mGyro[1];
    mData.gyroscope.z = rawData.mGyro[2];

    mData.angle.roll = rawData.mAngles[0];
    mData.angle.pitch = rawData.mAngles[1];
    mData.angle.yaw = rawData.mAngles[2];

    return mData;
}

bool ICM20948_Sensor::init()
{
    mInitialized = mImpl->initialize();
    
    if (mInitialized)
    {
        mImpl->calibrateGyro();
    }
    
    return mInitialized;
}

const std::string ICM20948_Sensor::whoAmI() const
{
    return "ICM20948";
}