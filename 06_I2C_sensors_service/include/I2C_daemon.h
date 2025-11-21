#ifndef I2C_GNSS_DAEMON_H
#define I2C_GNSS_DAEMON_H

#include "base_daemon.h"
#include "zhelpers.h"
#include "nlohmann.h"

#include "iIMUSensor.h"
#include "iLightSensor.h"
#include "iMeteoSensor.h"
#include "iProximitySensor.h"

#include <memory>
#include <string>

///////////////////////////////////////////////////////////////////////////////////

#define I2C_ZMQ_BUFFER_SIZE 1024

///////////////////////////////////////////////////////////////////////////////////

class i2cDaemon : public baseDaemon {

private:

	zmq::context_t m_context{1};

	std::shared_ptr<zmq::socket_t> m_zmqPUBer{nullptr};

	std::unique_ptr<iIMUSensor> m_imu{nullptr};
	std::unique_ptr<iLightSensor> m_lux{nullptr};
	std::unique_ptr<iProximitySensor> m_proxy{nullptr};
	std::unique_ptr<iMeteoSensor> m_meteo{nullptr};

	nlohmann::json m_json;

private:

	bool initZMQworkers();
	void sendDataToSubscribers(const std::string& serializedData);
	void stopZMQ();
	std::string serializeResult(const meteo::data& meteo, const light::data& light, 
		const proxy::data& proxy, const IMU::data& IMU);

public:

	i2cDaemon(const std::string inifilePath, const std::string serviceName);

	bool init();
	void run();
};

#endif /* I2C_GNSS_DAEMON_H */