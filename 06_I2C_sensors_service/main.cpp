#include "meteo_sensor_fabrique.h"
#include "light_sensor_fabrique.h"
#include "proxy_sensor_fabrique.h"
#include "imu_sensor_fabrique.h"

#include <iostream>

int main() {

	auto meteoSensor = meteoSensorFabrique{}.produceSensor(meteo::sensor_type::BME680, "/dev/i2c-1", 0x76);
	std::cout << "Meteo sensor used: " << meteoSensor->whoAmI() << std::endl;

	auto lightSensor = lightSensorFabrique{}.produceSensor(light::sensor_type::APDS9300, "/dev/i2c-1", 0x39);
	std::cout << "Light sensor used: " << lightSensor->whoAmI() << std::endl;

	auto proxySensor = proxySensorFabrique{}.produceSensor(proxy::sensor_type::APDS9960, "/dev/i2c-1", 0x4A);
	std::cout << "Proxy sensor used: " << proxySensor->whoAmI() << std::endl;

	auto imuSensor = imuSensorFabrique{}.produceSensor(IMU::sensor_type::ICM20948, "/dev/i2c-1", 0x86);
	std::cout << "IMU sensor used: " << imuSensor->whoAmI() << std::endl;

	return 0;
}