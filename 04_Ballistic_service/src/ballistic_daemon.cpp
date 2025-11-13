#include "ballistic_daemon.h"
#include "CFastLog.h"
#include "json_working_stuff.h"

#include <iostream>

ballisticDaemon::ballisticDaemon(const std::string inifilePath, const std::string serviceName) :  
baseDaemon(inifilePath, serviceName) {}

bool ballisticDaemon::initZMQworkers() {

	try {

		auto sendAddr = std::string("tcp://*:") + m_iniParser->getString("Zeromq_pub", "port", "5434");

		m_zmqPUBer = std::make_shared<zmq::socket_t>(m_context, ZMQ_PUB);
		m_zmqPUBer->bind(sendAddr);

		LOG_INFO(fastlog::LogEventType::System) << "Создан zmq-сокет (публикатор) по адресу: " << sendAddr;

		auto receiveAddr = std::string("tcp://*:") + m_iniParser->getString("Zeromq_pull", "port", "5433");
		m_zmqPULLer = std::make_shared<zmq::socket_t>(m_context, ZMQ_PULL);
		m_zmqPULLer->bind(receiveAddr);

		LOG_INFO(fastlog::LogEventType::System) << "Создан zmq-сокет (подписчик) по адресу: " << receiveAddr;
		
	}
	catch(const zmq::error_t& ex) {

		LOG_CRIT(fastlog::LogEventType::System) << "Ошибка в инициации сокетов zmq: [" << ex.what() << "]";
		return false;
	}	

	return true;
}

bool ballisticDaemon::init() {

	if(!initBaseDaemon()) {

		std::cerr << "Ошибка в инициализации базового демона!" << std::endl;
		return false;
	}

	if(!initZMQworkers()) {

		LOG_CRIT(fastlog::LogEventType::System) << "Не инициированы сокеты ZMQ! Аварийное завершение";
		return false;
	}

	initQueueThread();

	LOG_INFO(fastlog::LogEventType::System) << "Демон по рассчету баллистики успешно инициирован";
	return true;
}

void ballisticDaemon::initQueueThread() {

	LOG_INFO(fastlog::LogEventType::System) << "Поток для очереди сообщений занят в пулле";

	m_ThreadPool.push([this](int) {
		sendResultsToSubscribers();
	});
}

void ballisticDaemon::run() {

	LOG_INFO(fastlog::LogEventType::System) << "Демон по рассчету баллистики успешно стартовал";

	std::string incomingData;
	incomingData.reserve(BALLISTIX_WORKING_BUFFER_SIZE);

	std::vector<zmq::pollitem_t> items = {
		
		{static_cast<void*>(*m_zmqPULLer), 0, ZMQ_POLLIN, 0}
	};

	while(!canExit()) {

		int events = zmq::poll(items.data(), items.size(), 100);

		if (events > 0) {
			
			if (items[0].revents & ZMQ_POLLIN) {
				
				incomingData = s_recv(*m_zmqPULLer);

				m_ThreadPool.push([this, data = std::move(incomingData)](int) {

					std::string workingBuffer;
					s2::solveBallistics(data, workingBuffer);
					sendResultsToQueue(std::move(workingBuffer));
				});
			}
		}

		sleep(0UL);
	}

	stopZMQ();
}

void ballisticDaemon::sendResultsToQueue(std::string&& workingBuffer) {

	LOG_INFO(fastlog::LogEventType::System) << "Результаты расчета добавлены в очередь на отправку";
	m_resultsQueue.push(std::move(workingBuffer));
}

void ballisticDaemon::stopZMQ() {

	if(m_zmqPUBer) {

		m_zmqPUBer->close();
		m_zmqPUBer.reset();
	}

	if(m_zmqPULLer) {

		m_zmqPULLer->close();
		m_zmqPULLer.reset();
	}

	m_context.close();
}


void ballisticDaemon::sendResultsToSubscribers() {

	while(!canExit()) {

		while(auto result = m_resultsQueue.pop()) {
			
			LOG_INFO(fastlog::LogEventType::System) << "Результат расчета отправлен подписчикам:";
			LOG_INFO(fastlog::LogEventType::System) << *result;
			s_send(*m_zmqPUBer, *result, ZMQ_DONTWAIT);
		}

		sleep(0UL);
	}	
}