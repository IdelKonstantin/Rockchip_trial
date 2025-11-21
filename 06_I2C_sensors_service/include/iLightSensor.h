#ifndef _I_LIGHT_SENSOR_H_
#define _I_LIGHT_SENSOR_H_

#include <string>

namespace light {

	using lux_t = float;

	enum class level {

		TOTAL_DARKNESS,
		VERY_DARK,
		DARK,
		TWILIGHT,
		DIM_LIGHT,
		NORMAL_LIGHT,
		BRIGHT_LIGHT,
		VERY_BRIGHT
	};

	struct lightData {

		lux_t lightIntencity;
		level lightLevel;
	};

	using data = lightData;
};

class iLightSensor {

public:

	virtual const light::data& getLightData() = 0;
	virtual bool init() = 0;
	virtual const std::string whoAmI() const = 0;
	
	virtual ~iLightSensor() = default;
};

#endif /* _I_LIGHT_SENSOR_H_ */