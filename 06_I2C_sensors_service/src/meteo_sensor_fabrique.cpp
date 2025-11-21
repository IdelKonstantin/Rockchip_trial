#include "meteo_sensor_fabrique.h"

#include "BME280_Sensor.h"
#include "BME680_Sensor.h"

std::unique_ptr<iMeteoSensor> meteoSensorFabrique::produceSensor(meteo::sensor_type type, const std::string& i2c_device, uint8_t address) const {

	std::unique_ptr<iMeteoSensor> worker{nullptr};

	switch(type) {

		case meteo::sensor_type::BME280:
			worker = std::make_unique<BME280_Sensor>(i2c_device, address);
			break;

		case meteo::sensor_type::BME680:
			worker = std::make_unique<BME680_Sensor>(i2c_device, address);
			break;

		default:
			break;
	}

	return worker;
}