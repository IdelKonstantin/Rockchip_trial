#ifndef _I_METEO_SENSOR_H_
#define _I_METEO_SENSOR_H_

#include <cstdint>
#include <string>

namespace meteo {

	using temp_t = float;
	using press_t = uint16_t;
	using humid_t = uint8_t;
	using wind_speed_t = float;
	using wind_dir_t = uint16_t;

	struct meteoData {

		temp_t temperature;
		press_t pressure;
		humid_t humidity;
		wind_speed_t windSpeed;
		wind_dir_t windDirection;
	};

	using data = meteoData;
};

class iMeteoSensor {

public:

	virtual const meteo::data& getMeteoConditions() = 0;
	virtual bool init() = 0;
	virtual const std::string whoAmI() const = 0;

	virtual ~iMeteoSensor() = default;
};

#endif /* _I_METEO_SENSOR_H_ */