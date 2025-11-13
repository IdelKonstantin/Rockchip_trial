#include <iostream>

#include "base_daemon.h"

///////////////////////////////////////////////////////////////////////////////////////////////

/****** PUBLICS ******/

bool baseDaemon::m_canExit = true;

baseDaemon::baseDaemon(const std::string inifilePath, const std::string serviceName) : 
	m_iniParser{new INIParser()}, m_serviceName(serviceName) {

	signal(SIGINT, baseDaemon::signalHandler);
	signal(SIGTERM, baseDaemon::signalHandler);
	signal(SIGPIPE, SIG_IGN);
}

void baseDaemon::setThreadPoolSize(int nThreads) {

	m_ThreadPool.resize(nThreads);
}

baseDaemon::~baseDaemon() {

	LOG_INFO(fastlog::LogEventType::System) << "Базовый демон штатно завершен";
}

bool baseDaemon::initBaseDaemon() {

	if(!m_iniParser) {

		std::cerr << "Базовый демон [" << m_serviceName << "] не стартовал (m_iniParser == nullptr)" << std::endl;
		return false;
	}

	const std::string& configPath{"conf.ini"};

	if(!m_iniParser->load(configPath)) {

		std::cerr << "Базовый демон [" << m_serviceName << "] не стартовал, не прочитан конфиг [" << configPath << "]" << std::endl;
		return false;
	}

	std::string loggerDirPath = m_iniParser->getString("Logger", "dir", "./");

	if(!fs::exists(loggerDirPath)) {

		std::cerr << "Папка для логов по пути [" << loggerDirPath << "] не найдена" << std::endl;

		bool created = fs::create_directories(loggerDirPath);

		if(created) {
			std::cerr << "Папка по пути [" << loggerDirPath << "] создана" << std::endl;
		}
		else {
			std::cerr << "Не удалось создать папку по [" << loggerDirPath << "]. Путь для логов изменен на ./" << std::endl;
			loggerDirPath = "./";
		}
	}

	if(!logger::initLogSystem(loggerDirPath, m_serviceName)) {
		
		std::cerr << "Базовый демон [" << m_serviceName << "] не стартовал, не инициирован логгер по пути [" << loggerDirPath << "]" << std::endl;
		return false;
	}

	std::cerr << "Логи будут сохраняться в [" << loggerDirPath << "]" << std::endl;

	setThreadPoolSize(m_iniParser->getInt("Threads", "number", BASE_DAEMON_MAX_THREAD_POOL_SIZE));

	LOG_INFO(fastlog::LogEventType::System) << "Базовый демон штатно запустился";
	LOG_INFO(fastlog::LogEventType::System) << "Инициирован пулл потоков, размером [" << m_ThreadPool.size() << "]";

	m_canExit = false;
	return true;
}

void baseDaemon::signalHandler(int signal) {

	LOG_INFO(fastlog::LogEventType::System) << "Сработал обработчик сигналов.";

	switch(signal) {
	
		case SIGINT:
		case SIGTERM:
		{
			LOG_INFO(fastlog::LogEventType::System) << "Завершаем работу по сигналу.";
			forceExit();
			break;
		}
	};
}

bool baseDaemon::canExit() {

	return m_canExit;
}

/****** PRIVATES ******/

void baseDaemon::forceExit() {

	m_canExit = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool logger::initLogSystem(const std::string& loggerDirPath, const std::string& serviceName) {

	bool opened{false};

	if(fastlog::is_opened()) {

		fastlog::close_log();
	}

	try {

		fastlog::initialize(fastlog::GuaranteedLogger{}, loggerDirPath, serviceName, 1, "");
		fastlog::set_log_level(fastlog::LogLevel::INFO);
		fastlog::log_add_tag("");

		opened = fastlog::is_opened();
	}
	catch(const std::bad_alloc& ex) {

		std::cerr << "Не хватает свободной памяти. Причина [" << ex.what() << "]" << std::endl;
		opened = false;
	}

	return opened;
}