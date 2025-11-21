#include "light_sensor_fabrique.h"

#include "BH1750_Sensor.h"
#include "TSL2561_Sensor.h"
#include "APDS9930_Sensor.h"
#include "APDS9300_Sensor.h"

std::unique_ptr<iLightSensor> lightSensorFabrique::produceSensor(light::sensor_type type, const std::string& i2c_device, uint8_t address) const {

	std::unique_ptr<iLightSensor> worker{nullptr};

	switch(type) {

		case light::sensor_type::BH1750:
			worker = std::make_unique<BH1750_Sensor>(i2c_device, address);
			break;

		case light::sensor_type::TSL2561:
			worker = std::make_unique<TSL2561_Sensor>(i2c_device, address);
			break;

		case light::sensor_type::APDS9930:
			worker = std::make_unique<APDS9930_Sensor>(i2c_device, address);
			break;

		case light::sensor_type::APDS9300:
			worker = std::make_unique<APDS9300_Sensor>(i2c_device, address);
			break;

		default:
			break;
	}

	return worker;
}