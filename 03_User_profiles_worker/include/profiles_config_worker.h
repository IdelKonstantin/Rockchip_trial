#ifndef _USER_PROFILES_WORKER_H_
#define _USER_PROFILES_WORKER_H_

///////////////////////////////////////////////////////////////////////////////

#include <experimental/filesystem>
#include <fstream>
#include <functional>

#include "profiles_config.h"

///////////////////////////////////////////////////////////////////////////////

#define USER_PROFILES_CONFIG_NAME 			"user_profiles_config.json"	

///////////////////////////////////////////////////////////////////////////////

namespace fs = std::experimental::filesystem;

///////////////////////////////////////////////////////////////////////////////

class UserProfilesWorker {

private:
	
	user::TP_profile_info_t m_TPprofiles;
	user::TP_common_settings_t m_TPcommons;
	user::T_common_settings_HW_t m_TPHWcommons;

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

	UserProfilesWorker(const char* configPath = USER_PROFILES_CONFIG_NAME);

	user::TP_profile_info_t& getTPprofiles();
	user::TP_common_settings_t& getTPCommons();
	user::T_common_settings_HW_t getTPHWcommons();

	void setConfigWriteCallback(std::function<void(bool)> writeCallback);
	void setConfigReadCallback(std::function<void(bool)> readCallback);
	void setJsonValidationCallback(std::function<void()> jsonValidationFailureCallback);

	void loadFromFile();
	void saveToFile();
};

#endif /* _USER_PROFILES_WORKER_H_ */