#ifndef BALLISTIC_DAEMON_H
#define BALLISTIC_DAEMON_H

#include "base_daemon.h"
#include "zhelpers.h"
#include "simple_lockfree_queue.h"

///////////////////////////////////////////////////////////////////////////////////

#include <memory>
#include <mutex>

///////////////////////////////////////////////////////////////////////////////////

class ballisticDaemon : public baseDaemon {

private:

	zmq::context_t m_context{1};

	std::shared_ptr<zmq::socket_t> m_zmqPULLer{nullptr};
	std::shared_ptr<zmq::socket_t> m_zmqPUBer{nullptr};
	
	SimpleLockFreeQueue<std::string> m_resultsQueue;

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

#endif /* BALLISTIC_DAEMON_H */