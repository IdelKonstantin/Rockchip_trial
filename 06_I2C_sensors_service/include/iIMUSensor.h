#ifndef _I_IMU_SENSOR_H_
#define _I_IMU_SENSOR_H_

namespace IMU {

	using accel_t = float;
	using gyro_t = float;
	using angle_t = float;

	struct Acceleration {

		accel_t x;
		accel_t y;
		accel_t z;
	};

	struct Gyroscope {

		gyro_t x;
		gyro_t y;
		gyro_t z;
	};

	struct Angles {

		angle_t roll;
		angle_t pitch;
		angle_t yaw;
	};

	struct IMUData {

		accels acceleration;
		gyros gyroscope;
		angles angle;
	};

	using data = IMUData;
};

class iIMUSensor {

public:

	virtual const IMU::data& getIMUData() = 0;
	virtual bool init() = 0;
	virtual const std::string whoAmI() const = 0;

	virtual ~iIMUSensor() = default;
};

#endif /* _I_IMU_SENSOR_H_ */