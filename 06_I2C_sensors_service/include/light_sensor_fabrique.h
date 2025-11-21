#ifndef _LIGHT_SENSORS_FABRIQUE_H_
#define _LIGHT_SENSORS_FABRIQUE_H_

#include <memory>
#include <map>

#include "iLightSensor.h"

namespace light {

	enum class sensor_type : uint8_t {

		BH1750 = 0,
		TSL2561 = 1,
		APDS9930 = 2,
		APDS9300 = 3
	};

	const std::map<std::string, sensor_type> sensorTypeByName {
		
		{"BH1750", sensor_type::BH1750},
		{"TSL2561", sensor_type::TSL2561},
		{"APDS9930", sensor_type::APDS9930},
		{"APDS9300", sensor_type::APDS9300}
	};
};

class lightSensorFabrique final {

public:
	
	lightSensorFabrique() = default;
	std::unique_ptr<iLightSensor> produceSensor(light::sensor_type type, const std::string& i2c_device, uint8_t address) const;
};

#endif /* _LIGHT_SENSORS_FABRIQUE_H_ */