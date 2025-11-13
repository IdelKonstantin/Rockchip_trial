#ifndef _PERIPHERAL_SETTINGS_WORKER_H_
#define _PERIPHERAL_SETTINGS_WORKER_H_

///////////////////////////////////////////////////////////////////////////////

#include <experimental/filesystem>
#include <fstream>
#include <functional>

#include "peripheral_settings.h"

///////////////////////////////////////////////////////////////////////////////

#define PERIPH_CONFIG_NAME 			"peripherals_config.json"	

///////////////////////////////////////////////////////////////////////////////

namespace fs = std::experimental::filesystem;

///////////////////////////////////////////////////////////////////////////////

class PeripheralSettingsWorker {

private:
	
	dev_setting_t m_settings;
	const fs::path m_configPath;
	std::fstream m_file;
	std::function<void(bool)> m_writeCallback;
	std::function<void(bool)> m_readCallback;
	std::function<void()> m_jsonValidationFailureCallback;
	
private:
	
	bool validateFileAccess();
	void setSettingsToDefaults();
	void invokeWriteCallback(bool writeStatus);
	void invokeReadCallback(bool validationStatus);
	void invokeJsonValidationFailureCallback();

public:

	PeripheralSettingsWorker(const char* configPath = PERIPH_CONFIG_NAME);

	dev_setting_t& getSettings();

	void setConfigWriteCallback(std::function<void(bool)> writeCallback);
	void setConfigReadCallback(std::function<void(bool)> readCallback);
	void setJsonValidationCallback(std::function<void()> jsonValidationFailureCallback);

	void loadFromFile();
	void saveToFile();
};

#endif /* _PERIPHERAL_SETTINGS_WORKER_H_ */