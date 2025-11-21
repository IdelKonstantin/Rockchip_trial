#include "imu_sensor_fabrique.h"

#include "MPU6050_Sensor.h"
#include "ICM20948_Sensor.h"

std::unique_ptr<iIMUSensor> imuSensorFabrique::produceSensor(IMU::sensor_type type, const std::string& i2c_device, uint8_t address) const {

	std::unique_ptr<iIMUSensor> worker{nullptr};

	switch(type) {

		case IMU::sensor_type::MPU6050:
			worker = std::make_unique<MPU6050_Sensor>(i2c_device, address);
			break;

		case IMU::sensor_type::ICM20948:
			worker = std::make_unique<ICM20948_Sensor>(i2c_device, address);
			break;

		default:
			break;
	}

	return worker;
}