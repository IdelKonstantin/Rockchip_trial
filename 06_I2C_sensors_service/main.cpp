#include "meteo_sensor_fabrique.h"
#include "light_sensor_fabrique.h"

#include <iostream>

int main() {

	auto meteoSensor = meteoSensorFabrique{}.produceSensor(meteo::sensor_type::BME680, "/dev/i2c-1", 0x76);
	std::cout << "Meteo sensor used: " << meteoSensor->whoAmI() << std::endl;

	auto lightSensor = lightSensorFabrique{}.produceSensor(light::sensor_type::APDS9300, "/dev/i2c-1", 0x39);
	std::cout << "Meteo sensor used: " << lightSensor->whoAmI() << std::endl;

	return 0;
}