#ifndef _IMU_SENSORS_FABRIQUE_H_
#define _IMU_SENSORS_FABRIQUE_H_

#include <memory>
#include <map>

#include "iIMUSensor.h"

namespace IMU {

	enum class sensor_type : uint8_t {

		MPU6050 = 0,
		ICM20948 = 1
	};

	const std::map<std::string, sensor_type> sensorTypeByName {
		
		{"MPU6050", sensor_type::MPU6050},
		{"ICM20948", sensor_type::ICM20948}
	};
};

class imuSensorFabrique final {

public:
	
	imuSensorFabrique() = default;
	std::unique_ptr<iIMUSensor> produceSensor(IMU::sensor_type type, const std::string& i2c_device, uint8_t address) const;
};

#endif /* _IMU_SENSORS_FABRIQUE_H_ */