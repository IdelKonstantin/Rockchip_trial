#ifndef _METEO_SENSORS_FABRIQUE_H_
#define _METEO_SENSORS_FABRIQUE_H_

#include <memory>
#include <map>

#include "iMeteoSensor.h"

namespace meteo {

	enum class sensor_type : uint8_t {

		BME280 = 0,
		BME680 = 1
	};

	const std::map<std::string, sensor_type> sensorTypeByName {
		
		{"BME280", sensor_type::BME280},
		{"BME680", sensor_type::BME680}
	};
};

class meteoSensorFabrique final {

public:
	
	meteoSensorFabrique() = default;
	std::unique_ptr<iMeteoSensor> produceSensor(meteo::sensor_type type, const std::string& i2c_device, uint8_t address) const;
};

#endif /* _METEO_SENSORS_FABRIQUE_H_ */