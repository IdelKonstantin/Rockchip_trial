#include "proxy_sensor_fabrique.h"

#include "APDS9960_Sensor.h"
#include "VCNL4040_Sensor.h"
#include "MAX44009_Sensor.h"

std::unique_ptr<iProximitySensor> proxySensorFabrique::produceSensor(proxy::sensor_type type, const std::string& i2c_device, uint8_t address) const {

	std::unique_ptr<iProximitySensor> worker{nullptr};

	switch(type) {

		case proxy::sensor_type::APDS9960:
			worker = std::make_unique<APDS9960_Sensor>(i2c_device, address);
			break;

		case proxy::sensor_type::VCNL4040:
			worker = std::make_unique<VCNL4040_Sensor>(i2c_device, address);
			break;

		case proxy::sensor_type::MAX44009:
			worker = std::make_unique<MAX44009_Sensor>(i2c_device, address);
			break;

		default:
			break;
	}

	return worker;
}