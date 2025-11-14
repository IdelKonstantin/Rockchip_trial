#ifndef BASE_DAEMON_H
#define BASE_DAEMON_H

///////////////////////////////////////////////////////////////////////////////////////////////

#include <csignal>
#include <memory>
#include <vector>
#include <experimental/filesystem>

///////////////////////////////////////////////////////////////////////////////////////////////

#include "CFastLog.h"
#include "CThreadPool.h"
#include "ini_parser.h"

///////////////////////////////////////////////////////////////////////////////////////////////

namespace fs = std::experimental::filesystem;

///////////////////////////////////////////////////////////////////////////////////////////////

class logger {

public:
	static bool initLogSystem(const std::string& loggerDirPath, const std::string& serviceName);
};

///////////////////////////////////////////////////////////////////////////////////////////////

#define BASE_DAEMON_MAX_THREAD_POOL_SIZE 4

///////////////////////////////////////////////////////////////////////////////////////////////

class baseDaemon {

protected:

	std::unique_ptr<INIParser> m_iniParser{nullptr};
	const std::string m_configPath;
	const std::string m_serviceName;
	threadpool::CThreadPool m_ThreadPool;

protected:

	static void forceExit();
	void setThreadPoolSize(int nThreads);

public:

	static bool m_canExit;
	static void signalHandler(int signal);
	
	bool canExit();
	bool initBaseDaemon();

	baseDaemon(const std::string inifilePath, const std::string serviceName);
	virtual ~baseDaemon();
};

#endif /* BASE_DAEMON_H */
