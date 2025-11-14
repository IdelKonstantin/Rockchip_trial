#ifndef GPS_GNSS_DAEMON_H
#define GPS_GNSS_DAEMON_H

#include "base_daemon.h"
#include "zhelpers.h"
#include "GPS_worker.h"

///////////////////////////////////////////////////////////////////////////////////
//
//	TODO list: 
//	1) В gpsDaemon::powerONAndStartGPSModule() сделать "железную" инициацию модуля
//	2) В gpsDaemon::powerOFFGPSModule() сделать отключение ключа питания 
//
///////////////////////////////////////////////////////////////////////////////////

#include <memory>

///////////////////////////////////////////////////////////////////////////////////

#define GPS_DATA_OUTPUT_BUFFER_SIZE 0x400

///////////////////////////////////////////////////////////////////////////////////

class gpsDaemon : public baseDaemon {

private:

	zmq::context_t m_context{1};

	std::shared_ptr<zmq::socket_t> m_zmqPULLer{nullptr};
	std::shared_ptr<zmq::socket_t> m_zmqPUBer{nullptr};

	std::unique_ptr<GPSWorker> m_gps{nullptr};

	std::atomic<bool> m_GPSisActive{false};

private:
	void powerONAndStartGPSModule();
	void powerOFFGPSModule();
	bool initZMQworkers();
	void statupGPSInit();
	void initSwithcherThread();

	void processIncomingCommand();
	void sendGPSDataToSubscribers(const std::string& GPSSerializedData);
	void stopZMQ();
	void saveGPSStateInConfig();

public:

	gpsDaemon(const std::string inifilePath, const std::string serviceName);

	bool init();
	void run();
};

#endif /* GPS_GNSS_DAEMON_H */