#include "GPS_daemon.h"
#include "CFastLog.h"

#include <iostream>

void gpsDaemon::powerONAndStartGPSModule() {
		
	// TODO: Подать питание на ключ
	// возможно еще как-то инциировать модуль AT-командами

	LOG_INFO(fastlog::LogEventType::System) << "GPS модуль включен";	
}

void gpsDaemon::powerOFFGPSModule() {

	// TODO: Убрать питание с ключа
	LOG_INFO(fastlog::LogEventType::System) << "GPS модуль выключен";	
}

gpsDaemon::gpsDaemon(const std::string inifilePath, const std::string serviceName) :  
baseDaemon(inifilePath, serviceName) {}

bool gpsDaemon::initZMQworkers() {

	try {

		auto sendAddr = std::string("tcp://*:") + m_iniParser->getString("Zeromq_pub", "port", "5435");

		m_zmqPUBer = std::make_shared<zmq::socket_t>(m_context, ZMQ_PUB);
		m_zmqPUBer->bind(sendAddr);

		LOG_INFO(fastlog::LogEventType::System) << "Создан zmq-сокет (публикатор) по адресу: " << sendAddr;

		auto receiveAddr = std::string("tcp://*:") + m_iniParser->getString("Zeromq_pull", "port", "5436");
		m_zmqPULLer = std::make_shared<zmq::socket_t>(m_context, ZMQ_PULL);
		m_zmqPULLer->bind(receiveAddr);

		LOG_INFO(fastlog::LogEventType::System) << "Создан zmq-сокет (пуллер) по адресу: " << receiveAddr;
		
	}
	catch(const zmq::error_t& ex) {

		LOG_CRIT(fastlog::LogEventType::System) << "Ошибка в инициации сокетов zmq: [" << ex.what() << "]";
		return false;
	}	

	return true;
}

void gpsDaemon::stopZMQ() {

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

void gpsDaemon::statupGPSInit() {

	bool startupGPSState = m_iniParser->getBool("GPS", "state", false);
	m_GPSisActive.store(startupGPSState);

	LOG_INFO(fastlog::LogEventType::System) << "При запуске GPS имел статус [" 
	<< (startupGPSState ? "ВКЛ" : "ВЫКЛ") << "]";

	if(m_GPSisActive.load()) {

		powerONAndStartGPSModule();
	}
	else {

		powerOFFGPSModule();
	}
}

void gpsDaemon::saveGPSStateInConfig() {

	m_iniParser->setValue("GPS", "state", std::string(m_GPSisActive.load() ? "true" : "false"));
	m_iniParser->save(m_configPath);
}

void gpsDaemon::processIncomingCommand() {

	LOG_INFO(fastlog::LogEventType::System) << "Инициирован поток в пулле для обработки команды ВКЛ/ВЫКЛ";

	std::string incomingData;

	std::vector<zmq::pollitem_t> items = {
		
		{static_cast<void*>(*m_zmqPULLer), 0, ZMQ_POLLIN, 0}
	};

	while(!canExit()) {

		int events = zmq::poll(items.data(), items.size(), 100);

		if (events > 0) {			
			
			if (items[0].revents & ZMQ_POLLIN) {
				
				incomingData = s_recv(*m_zmqPULLer);

				if(incomingData == "ON") {

					m_GPSisActive.store(true);
				}
				else if(incomingData == "OFF") {

					m_GPSisActive.store(false);
				}

				saveGPSStateInConfig();
			}
		}

		sleep(0UL);
	}	
}

void gpsDaemon::initSwithcherThread() {

	m_ThreadPool.push([this](int) {
		
		processIncomingCommand();
	});	
}

bool gpsDaemon::init() {

	if(!initBaseDaemon()) {

		std::cerr << "Ошибка в инициализации базового демона!" << std::endl;
		return false;
	}

	try {
		
		m_gps = std::make_unique<GPSWorker>(m_iniParser->getString("GPS", "file", "/dev/ttyUSB0"), m_iniParser->getInt("GPS", "baud", 38400));

		if(!m_gps->initialize()) {

			LOG_CRIT(fastlog::LogEventType::System) << "Ошибка в файловом дескрипторе GPS";
			return false;
		}

		LOG_INFO(fastlog::LogEventType::System) << "Файловый дескриптор для работы с GPS открыт";

	}
	catch(const std::exception& ex) {

		LOG_CRIT(fastlog::LogEventType::System) << "Ошибка в файловом дескрипторе GPS. Причина: " << ex.what();
		return false;
	}

	statupGPSInit();

	if(!initZMQworkers()) {

		LOG_CRIT(fastlog::LogEventType::System) << "Не инициированы сокеты ZMQ! Аварийное завершение";
		return false;
	}

	initSwithcherThread();

	LOG_INFO(fastlog::LogEventType::System) << "Демон для работы с GPS успешно инициирован";
	return true;
}

void gpsDaemon::run() {

	LOG_INFO(fastlog::LogEventType::System) << "Демон для работы с модулем GPS успешно стартовал";

	while(!canExit()) {

		if(m_GPSisActive.load()) {

			m_gps->processData();	
			auto result = m_gps->serializeResult(m_gps->getGPSData());
			s_send(*m_zmqPUBer, result, ZMQ_DONTWAIT);
		}

		sleep(0UL);
	}

	m_gps->cleanup();
	stopZMQ();
}

void gpsDaemon::sendGPSDataToSubscribers(const std::string& GPSSerializedData) {

	s_send(*m_zmqPUBer, GPSSerializedData, ZMQ_DONTWAIT);
}