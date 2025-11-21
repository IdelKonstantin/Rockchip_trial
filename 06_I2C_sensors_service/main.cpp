#include "meteo_sensor_fabrique.h"
#include "light_sensor_fabrique.h"
#include "proxy_sensor_fabrique.h"

#include "MPU6050_Sensor.h"

#include <iostream>

int main() {

	auto meteoSensor = meteoSensorFabrique{}.produceSensor(meteo::sensor_type::BME680, "/dev/i2c-1", 0x76);
	std::cout << "Meteo sensor used: " << meteoSensor->whoAmI() << std::endl;

	auto lightSensor = lightSensorFabrique{}.produceSensor(light::sensor_type::APDS9300, "/dev/i2c-1", 0x39);
	std::cout << "Light sensor used: " << lightSensor->whoAmI() << std::endl;

	auto proxySensor = proxySensorFabrique{}.produceSensor(proxy::sensor_type::APDS9960, "/dev/i2c-1", 0x4A);
	std::cout << "Proxy sensor used: " << proxySensor->whoAmI() << std::endl;

	MPU6050_Sensor imu;

	std::cout << imu.whoAmI() << std::endl;

	return 0;
}