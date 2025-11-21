#ifndef _PROXIMITY_SENSORS_FABRIQUE_H_
#define _PROXIMITY_SENSORS_FABRIQUE_H_

#include <memory>
#include <map>

#include "iProximitySensor.h"

namespace proxy {

	enum class sensor_type : uint8_t {

		APDS9960 = 0,
		VCNL4040 = 1,
		MAX44009 = 2
	};

	const std::map<std::string, sensor_type> sensorTypeByName {
		
		{"APDS9960", sensor_type::APDS9960},
		{"VCNL4040", sensor_type::VCNL4040},
		{"MAX44009", sensor_type::MAX44009}
	};
};

class proxySensorFabrique final {

public:
	
	proxySensorFabrique() = default;
	std::unique_ptr<iProximitySensor> produceSensor(proxy::sensor_type type, const std::string& i2c_device, uint8_t address) const;
};

#endif /* _PROXIMITY_SENSORS_FABRIQUE_H_ */