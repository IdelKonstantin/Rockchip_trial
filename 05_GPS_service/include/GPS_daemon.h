#ifndef GPS_GNSS_DAEMON_H
#define GPS_GNSS_DAEMON_H

#include "base_daemon.h"
#include "zhelpers.h"

///////////////////////////////////////////////////////////////////////////////////

#include <memory>

///////////////////////////////////////////////////////////////////////////////////

class gpsDaemon : public baseDaemon {

private:

	zmq::context_t m_context{1};

	std::shared_ptr<zmq::socket_t> m_zmqPULLer{nullptr};
	std::shared_ptr<zmq::socket_t> m_zmqPUBer{nullptr};

	std::atomic<bool> m_GPSisActive{false};

private:

	bool initZMQworkers();
	void initSwithcherThread();
	void sendResultsToSubscribers();
	void stopZMQ();

	void statupGPSInit();

public:

	gpsDaemon(const std::string inifilePath, const std::string serviceName);

	bool init();
	void run();
};

#endif /* GPS_GNSS_DAEMON_H */