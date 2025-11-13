#ifndef _BALLISTIX_SETTINGS_WORKER_H_
#define _BALLISTIX_SETTINGS_WORKER_H_
 
///////////////////////////////////////////////////////////////////////////////

#include <experimental/filesystem>
#include <fstream>
#include <functional>

#include "ballistics_config.h"

///////////////////////////////////////////////////////////////////////////////

#define BALLISTICS_CONFIG_NAME 			"ballistics_config.json"
#define BALLISTICS_JSON_BUFFER_SIZE		16384

///////////////////////////////////////////////////////////////////////////////

namespace fs = std::experimental::filesystem;

///////////////////////////////////////////////////////////////////////////////

/******************************************************
 * 
 *  Class for parsing json configs relative to 
 *  ballistic calculations
 * 
 * ***************************************************/

class configWorker {

private:

	std::string m_jsonString;
	const fs::path m_configPath;
	std::fstream m_file;

	ballistics::bullets_info_t m_bullets;
	ballistics::rifles_info_t m_rifles;
	the_device::settings_t m_settings;
	BC::inputs_t m_inputs;
	BC::target_info_t m_target;
	BC::mildot_inputs_t m_mildot;

	const std::map<std::string, uint8_t> m_DFs {

		{"G1", G1},
		{"G7", G7},
		{"Gs", Gs},
		{"CDM", CDM},
		{"MBCG1", MBCG1},
		{"MBCG7", MBCG7}
	};

	std::function<void(bool)> m_writeCallback;
	std::function<void(bool)> m_readCallback;
	std::function<void()> m_jsonValidationFailureCallback;

private:

	void parseContent();
	void contentToJson();

	bool validateFileAccess();
	void setSettingsToDefaults();
	void invokeWriteCallback(bool writeStatus);
	void invokeReadCallback(bool validationStatus);
	void invokeJsonValidationFailureCallback();

	const std::string getDFNameByDFType(uint8_t DFtype);

public:
	configWorker(const char* configPath = BALLISTICS_CONFIG_NAME);

	void setConfigWriteCallback(std::function<void(bool)> writeCallback);
	void setConfigReadCallback(std::function<void(bool)> readCallback);
	void setJsonValidationCallback(std::function<void()> jsonValidationFailureCallback);

	/********** Config reading-parsing-processing ************/

	void readConfig();
	void saveConfig();

	/********** Bullets parsing/processing *******************/

	ballistics::bullets_info_t& getBulletsInfo();
	ballistics::bullet_t& getCurrentBulletInfo();

	/********** Rifles parsing/processing ********************/

	ballistics::rifles_info_t& getRiflesInfo();
	ballistics::rifle_t& getCurrentRifleInfo();

	/********** Divice settings parsing/processing ***********/

	the_device::settings_t& getDeviceSettings();

	/********** BC inputs parsing/processing *****************/

	BC::inputs_t& getBCInputs();

	/********** BC target data parsing/processing ************/

	BC::target_info_t& getTargetData();

	/********** BC mildot inputs parsing/processing **********/

	BC::mildot_inputs_t& getMildotInputs();
};

#endif /* _BALLISTIX_SETTINGS_WORKER_H_ */