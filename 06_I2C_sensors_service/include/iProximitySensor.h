#ifndef _I_PROXIMITY_SENSOR_H_
#define _I_PROXIMITY_SENSOR_H_

#include <string>

namespace proxy {

	using proximity_t = bool;

	struct priximityData {

		proximity_t proximity;
	};

	using data = priximityData;
};

class iProximitySensor {

public:

	virtual const proxy::data& getProximityStatus() = 0;
	virtual bool init() = 0;
	virtual const std::string whoAmI() const = 0;

	virtual ~iProximitySensor() = default;
};

#endif /* _I_PROXIMITY_SENSOR_H_ */