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

private:

	bool initZMQworkers();
	void initQueueThread();
	void sendResultsToQueue(std::string&& workingBuffer);
	void sendResultsToSubscribers();
	void stopZMQ();

public:

	ballisticDaemon(const std::string inifilePath, const std::string serviceName);

	bool init();
	void run();
};

#endif /* GPS_GNSS_DAEMON_H */