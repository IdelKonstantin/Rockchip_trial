#include <cstring>
#include <string>

///////////////////////////////////////////////////////////////////////////////

#include "peripherals_settings_worker.h"
#include "nlohmann.h"

///////////////////////////////////////////////////////////////////////////////
	
PeripheralSettingsWorker::PeripheralSettingsWorker(const char* configPath) : m_configPath(std::string(configPath)) {}

bool PeripheralSettingsWorker::validateFileAccess() {

	if (!fs::exists(m_configPath) || (!fs::is_regular_file(m_configPath))) {
		
		return false;
	}
	
	return true;
}

void PeripheralSettingsWorker::setSettingsToDefaults() {

	m_settings.lrf.OFFtime 				=	DEFAULT_LRF_OFF_TIME_SEC;
	m_settings.lrf.measureTime 			=	DEFAULT_LRF_MEASURE_TIMEOUT_SEC;
	m_settings.lrf.bufferShow 			=	DEFAULT_LRF_BUFFER_SHOW;

	m_settings.compass.active 			=	DEFAULT_COMPASS_STATE;
	m_settings.compass.magDeclination 	=	DEFAULT_COMPASS_MAG_DECL;
	memset(&m_settings.compass.calibration, 0, sizeof(mag_calibration_t));

	m_settings.gps.active				=	DEFAULT_GNSS_STATE;
	m_settings.gps.timeZone				=	DEFAULT_GNSS_TIMEZONE;
	m_settings.gps.isDst				=	DEFAULT_GNSS_IS_DIST;

	m_settings.buttons.shift 			=	DEFAULT_SHIFT_KEY_STATE;
	m_settings.buttons.FN 				=	DEFAULT_FN_KEY_STATE;
	m_settings.buttons.engeeniring 		=	DEFAULT_ENGEENIRING_STATE;

	m_settings.perif.menuShowtime		=	DEFAULT_MENU_OFF_TIME_SEC;
	m_settings.perif.tipsShow			=	DEFAULT_PERIF_TIPS_SHOW;
	m_settings.perif.anglesShow			=	DEFAULT_PERIF_ANGLES_SHOW;
	m_settings.perif.meteoShow			=	DEFAULT_PERIF_METEO_SHOW;
}

void PeripheralSettingsWorker::invokeWriteCallback(bool writeStatus) {

	if(m_writeCallback) {
		m_writeCallback(writeStatus);
	}		
}

void PeripheralSettingsWorker::invokeReadCallback(bool validationStatus) {

	if(m_readCallback) {
		m_readCallback(validationStatus);
	}		
}

void PeripheralSettingsWorker::invokeJsonValidationFailureCallback() {

	if(m_jsonValidationFailureCallback) {
		m_jsonValidationFailureCallback();
	}
}

dev_setting_t& PeripheralSettingsWorker::getSettings() {
	
	return m_settings;
}

void PeripheralSettingsWorker::setConfigWriteCallback(std::function<void(bool)> writeCallback) {
	
	m_writeCallback = writeCallback;
}

void PeripheralSettingsWorker::setConfigReadCallback(std::function<void(bool)> readCallback) {
	
	m_readCallback = readCallback;
}

void PeripheralSettingsWorker::setJsonValidationCallback(std::function<void()> jsonValidationFailureCallback) {
	
	m_jsonValidationFailureCallback = jsonValidationFailureCallback;
}	

void PeripheralSettingsWorker::loadFromFile() {

	/***********************************************************************************************************
	 * 
	 *  Если в файловой системе отсутствует конфиг с настройками периферии, то надо инициировать все дефолтными 
	 *  значениями и сохранить новый конфиг-файл  
	 * 
	 * ********************************************************************************************************/

	if (!validateFileAccess()) {
		
		setSettingsToDefaults();
		saveToFile();
		return;
	}

	/***********************************************************************************************************
	 * 
	 *  Если же конфиг присутствует, то надо прочитать его, проверить валидный ли JSON и если не валидный, то
	 * 	выйти, в противном случае -> десерилизовать в структуру m_settings
	 * 
	 * ********************************************************************************************************/

	m_file.open(m_configPath.string(), std::ios::in);

	bool readStatus = m_file.is_open();

	if(!readStatus) {

		invokeReadCallback(readStatus);
		return;
	}

	try {

		auto bodyJson = nlohmann::json::parse(m_file);

		m_settings.lrf.OFFtime 				=	bodyJson["LRF"]["timeout"].get<decltype(LRF_setting_t::OFFtime)>();
		m_settings.lrf.measureTime 			=	bodyJson["LRF"]["measurment_gap_time"].get<decltype(LRF_setting_t::measureTime)>();
		m_settings.lrf.bufferShow 			=	bodyJson["LRF"]["show_distances"].get<decltype(LRF_setting_t::bufferShow)>();

		m_settings.compass.active 			=	bodyJson["Compass"]["active"].get<decltype(MAG_setting_t::active)>();
		m_settings.compass.magDeclination 	=	bodyJson["Compass"]["dectination"].get<decltype(MAG_setting_t::magDeclination)>();

		m_settings.compass.calibration.offset[0] = bodyJson["Compass"]["calibration"]["offset"][0].get<mag::calibration_unit_t>();
		m_settings.compass.calibration.offset[1] = bodyJson["Compass"]["calibration"]["offset"][1].get<mag::calibration_unit_t>();
		m_settings.compass.calibration.offset[2] = bodyJson["Compass"]["calibration"]["offset"][2].get<mag::calibration_unit_t>();

		m_settings.compass.calibration.scale[0] = bodyJson["Compass"]["calibration"]["scale"][0].get<mag::calibration_unit_t>();
		m_settings.compass.calibration.scale[1] = bodyJson["Compass"]["calibration"]["scale"][1].get<mag::calibration_unit_t>();
		m_settings.compass.calibration.scale[2] = bodyJson["Compass"]["calibration"]["scale"][2].get<mag::calibration_unit_t>();

		m_settings.gps.active				=	bodyJson["GNSS"]["active"].get<decltype(GPS_setting_t::active)>();
		m_settings.gps.timeZone				=	bodyJson["GNSS"]["timezone"].get<decltype(GPS_setting_t::timeZone)>();
		m_settings.gps.isDst				=	bodyJson["GNSS"]["is_dst"].get<decltype(GPS_setting_t::isDst)>();

		m_settings.buttons.shift 			=	bodyJson["Buttons"]["shift_active"].get<decltype(BTN_setting_t::shift)>();
		m_settings.buttons.FN 				=	bodyJson["Buttons"]["FN_status"].get<decltype(BTN_setting_t::FN)>();
		m_settings.buttons.engeeniring 		=	bodyJson["Buttons"]["engeeniring_menu"].get<decltype(BTN_setting_t::engeeniring)>();

		m_settings.perif.menuShowtime		=	bodyJson["Interface"]["menu_timeout"].get<decltype(perif_settings_t::menuShowtime)>();
		m_settings.perif.tipsShow			=	bodyJson["Interface"]["tips_show"].get<decltype(perif_settings_t::tipsShow)>();
		m_settings.perif.anglesShow			=	bodyJson["Interface"]["angles_show"].get<decltype(perif_settings_t::anglesShow)>();
		m_settings.perif.meteoShow			=	bodyJson["Interface"]["meteo_show"].get<decltype(perif_settings_t::meteoShow)>();
		m_file.close();
		invokeReadCallback(readStatus);
		return;
	}
	catch(...) {

		m_file.close();
		setSettingsToDefaults();
		invokeJsonValidationFailureCallback();
		invokeReadCallback(false);
		return;
	}
}

void PeripheralSettingsWorker::saveToFile() {

	m_file.open(m_configPath.string(), std::ios::out | std::ios::trunc);
	
	bool writeStatus = m_file.is_open();

	if(!writeStatus) {
		invokeWriteCallback(writeStatus);
		return;
	}

	nlohmann::json responceJson;
	
	responceJson["LRF"]["timeout"] 						= m_settings.lrf.OFFtime;
	responceJson["LRF"]["measurment_gap_time"]			= m_settings.lrf.measureTime;
	responceJson["LRF"]["show_distances"]				= m_settings.lrf.bufferShow;

	responceJson["Compass"]["active"]					= m_settings.compass.active;
	responceJson["Compass"]["dectination"]				= m_settings.compass.magDeclination;
	
	responceJson["Compass"]["calibration"]["offset"]	= {
		
		m_settings.compass.calibration.offset[0], 
		m_settings.compass.calibration.offset[1], 
		m_settings.compass.calibration.offset[2]
	};

	responceJson["Compass"]["calibration"]["scale"]		= {

		m_settings.compass.calibration.scale[0], 
		m_settings.compass.calibration.scale[1], 
		m_settings.compass.calibration.scale[2]			
	};

	responceJson["GNSS"]["active"]						= m_settings.gps.active;
	responceJson["GNSS"]["timezone"]					= m_settings.gps.timeZone;
	responceJson["GNSS"]["is_dst"]						= m_settings.gps.isDst;

	responceJson["Buttons"]["shift_active"]				= m_settings.buttons.shift;
	responceJson["Buttons"]["FN_status"]				= m_settings.buttons.FN;
	responceJson["Buttons"]["engeeniring_menu"]			= m_settings.buttons.engeeniring;

	responceJson["Interface"]["menu_timeout"]			= m_settings.perif.menuShowtime;
	responceJson["Interface"]["tips_show"]				= m_settings.perif.tipsShow;
	responceJson["Interface"]["angles_show"]			= m_settings.perif.anglesShow;
	responceJson["Interface"]["meteo_show"]				= m_settings.perif.meteoShow;

	m_file << responceJson.dump(4);
	m_file.close();

	invokeWriteCallback(writeStatus);
}

bool operator!=(const dev_setting_t& lhs, const dev_setting_t& rhs) {

	return  lhs.lrf.OFFtime != rhs.lrf.OFFtime ||
		lhs.lrf.measureTime != rhs.lrf.measureTime ||
		lhs.lrf.bufferShow != rhs.lrf.bufferShow ||

		lhs.compass.active != rhs.compass.active ||
		lhs.compass.magDeclination != rhs.compass.magDeclination ||

		lhs.compass.calibration.offset[0] != rhs.compass.calibration.offset[0] ||
		lhs.compass.calibration.offset[1] != rhs.compass.calibration.offset[1] ||
		lhs.compass.calibration.offset[2] != rhs.compass.calibration.offset[2] ||
		lhs.compass.calibration.scale[0] != rhs.compass.calibration.scale[0] ||
		lhs.compass.calibration.scale[1] != rhs.compass.calibration.scale[1] ||
		lhs.compass.calibration.scale[2] != rhs.compass.calibration.scale[2] ||
		lhs.compass.calibration.temp_coeff_offset[0] != rhs.compass.calibration.temp_coeff_offset[0] ||
		lhs.compass.calibration.temp_coeff_offset[1] != rhs.compass.calibration.temp_coeff_offset[1] ||
		lhs.compass.calibration.temp_coeff_offset[2] != rhs.compass.calibration.temp_coeff_offset[2] ||
		lhs.compass.calibration.temp_coeff_scale[0] != rhs.compass.calibration.temp_coeff_scale[0] ||
		lhs.compass.calibration.temp_coeff_scale[1] != rhs.compass.calibration.temp_coeff_scale[1] ||
		lhs.compass.calibration.temp_coeff_scale[2] != rhs.compass.calibration.temp_coeff_scale[2] ||
		lhs.compass.calibration.cal_temp != rhs.compass.calibration.cal_temp ||

		lhs.gps.active != rhs.gps.active ||
		lhs.gps.timeZone != rhs.gps.timeZone ||
		lhs.gps.isDst != rhs.gps.isDst ||

		lhs.buttons.shift != rhs.buttons.shift ||
		lhs.buttons.FN != rhs.buttons.FN ||
		lhs.buttons.engeeniring != rhs.buttons.engeeniring ||

		lhs.perif.menuShowtime != rhs.perif.menuShowtime ||
		lhs.perif.tipsShow != rhs.perif.tipsShow ||
		lhs.perif.anglesShow != rhs.perif.anglesShow ||
		lhs.perif.meteoShow != rhs.perif.meteoShow;
}