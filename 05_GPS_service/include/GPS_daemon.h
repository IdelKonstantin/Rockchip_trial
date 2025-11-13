#ifndef GPS_GNSS_DAEMON_H
#define GPS_GNSS_DAEMON_H

#include "base_daemon.h"
#include "zhelpers.h"

///////////////////////////////////////////////////////////////////////////////////
//
//	TODO list: В gpsDaemon::statupGPSInit() сделать "железную" инициацию модуля
//
///////////////////////////////////////////////////////////////////////////////////

#include <memory>

///////////////////////////////////////////////////////////////////////////////////

class gpsDaemon : public baseDaemon {

private:

	zmq::context_t m_context{1};

	std::shared_ptr<zmq::socket_t> m_zmqSUBer{nullptr};
	std::shared_ptr<zmq::socket_t> m_zmqPUBer{nullptr};

	std::atomic<bool> m_GPSisActive{false};

private:

	bool initZMQworkers();
	void statupGPSInit();
	void initSwithcherThread();

	void processIncomingCommand();
	void sendResultsToSubscribers();
	void stopZMQ();

public:

	gpsDaemon(const std::string inifilePath, const std::string serviceName);

	bool init();
	void run();
};

#endif /* GPS_GNSS_DAEMON_H */