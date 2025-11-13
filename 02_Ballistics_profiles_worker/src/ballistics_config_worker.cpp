#include <cmath>

///////////////////////////////////////////////////////////////////////////////

#include "ballistics_config_worker.h"
#include "nlohmann.h"

///////////////////////////////////////////////////////////////////////////////

bool ballistics::bullet::operator!=(const ballistics::bullet& other) const {

	const auto epsilon = FLOAT_EPSILON;

	return name != other.name ||
		DF != other.DF ||
		std::abs(BC - other.BC) > epsilon ||
		MV != other.MV ||
		std::abs(length - other.length) > epsilon ||
		weight != other.weight ||
		std::abs(caliber - other.caliber) > epsilon ||
		MV_temp != other.MV_temp ||
		std::abs(thermSens - other.thermSens) > epsilon ||
		std::abs(CF_M0_9 - other.CF_M0_9) > epsilon ||
		std::abs(CF_M1_0 - other.CF_M1_0) > epsilon ||
		std::abs(CF_M1_1 - other.CF_M1_1) > epsilon;
}

ballistics::bullet& ballistics::bullet::operator=(const ballistics::bullet& other) {

	if (this != &other) {

		name = other.name;
		DF = other.DF;
		BC = other.BC;
		MV = other.MV;
		length = other.length;
		weight = other.weight;
		caliber = other.caliber;
		MV_temp = other.MV_temp;
		thermSens = other.thermSens;
		CF_M0_9 = other.CF_M0_9;
		CF_M1_0 = other.CF_M1_0;
		CF_M1_1 = other.CF_M1_1;
	}

	return *this;
}

bool ballistics::rifle::operator!=(const ballistics::rifle& other) const {
        
	const auto epsilon = FLOAT_EPSILON;

	return name != other.name ||
		zero_dist != other.zero_dist ||
		std::abs(scope_hight - other.scope_hight) > epsilon ||
		std::abs(twist - other.twist) > epsilon ||
		twist_dir != other.twist_dir ||
		zeroing != other.zeroing ||
		zero_T != other.zero_T ||
		zero_P != other.zero_P ||
		std::abs(vert_drift - other.vert_drift) > epsilon ||
		vert_drift_dir != other.vert_drift_dir ||
		std::abs(horiz_drift - other.horiz_drift) > epsilon ||
		horiz_drift_dir != other.horiz_drift_dir ||
		scope_units != other.scope_units ||
		std::abs(vert_click - other.vert_click) > epsilon ||
		std::abs(horiz_click - other.horiz_click) > epsilon;
}

ballistics::rifle& ballistics::rifle::operator=(const ballistics::rifle& other) {

	if (this != &other) {

		name = other.name;
		zero_dist = other.zero_dist;
		scope_hight = other.scope_hight;
		twist = other.twist;
		twist_dir = other.twist_dir;
		zeroing = other.zeroing;
		zero_T = other.zero_T;
		zero_P = other.zero_P;
		vert_drift = other.vert_drift;
		vert_drift_dir = other.vert_drift_dir;
		horiz_drift = other.horiz_drift;
		horiz_drift_dir = other.horiz_drift_dir;
		scope_units = other.scope_units;
		vert_click = other.vert_click;
		horiz_click = other.horiz_click;
	}

	return *this;
}

bool the_device::settings::operator!=(const the_device::settings& other) const {
        
	const auto epsilon = FLOAT_EPSILON;

	return std::abs(latitude - other.latitude) > epsilon || 
		std::abs(magIncl - other.magIncl) > epsilon;
}

the_device::settings& the_device::settings::operator=(const the_device::settings& other) {

	if (this != &other) {

		latitude = other.latitude;
		magIncl = other.magIncl;
	}

	return *this;
}

bool BC::inputs::operator!=(const BC::inputs& other) const {
    
	return koriolis != other.koriolis ||
		termoCorr != other.termoCorr ||
		rangeCard != other.rangeCard ||
		aeroJump != other.aeroJump;
}

BC::inputs& BC::inputs::operator=(const BC::inputs& other) {

	if (this != &other) {

		koriolis = other.koriolis;
		termoCorr = other.termoCorr;
		rangeCard = other.rangeCard;
		aeroJump = other.aeroJump;
	}

	return *this;
}

bool BC::target::operator!=(const BC::target& other) const {
    
	const auto epsilon = FLOAT_EPSILON;

	return distance != other.distance ||
		terrainAngle != other.terrainAngle ||
		std::abs(speedMILs - other.speedMILs) > epsilon ||
		azimuth != other.azimuth ||
		windAngle != other.windAngle ||
		roll_angle != other.roll_angle;
}

BC::target& BC::target::operator=(const BC::target& other) {

	if (this != &other) {

		distance = other.distance;
		terrainAngle = other.terrainAngle;
		speedMILs = other.speedMILs;
		azimuth = other.azimuth;
		windAngle = other.windAngle;
		roll_angle = other.roll_angle;
	}

	return *this;
}

bool BC::mildot::operator!=(const BC::mildot& other) const {

	const auto epsilon = FLOAT_EPSILON;

	return std::abs(sizeMeters - other.sizeMeters) > epsilon || 
		std::abs(sizeMils - other.sizeMils) > epsilon;
}

BC::mildot& BC::mildot::operator=(const BC::mildot& other) {

	if (this != &other) {

		sizeMeters = other.sizeMeters;
		sizeMils = other.sizeMils;
	}

	return *this;
}

///////////////////////////////////////////////////////////////////////////////

configWorker::configWorker(const char* configPath) : m_configPath(std::string(configPath)) {}

void configWorker::setConfigWriteCallback(std::function<void(bool)> writeCallback) {

	m_writeCallback = writeCallback;
}

void configWorker::setConfigReadCallback(std::function<void(bool)> readCallback) {

	m_readCallback = readCallback;
}

void configWorker::setJsonValidationCallback(std::function<void()> jsonValidationFailureCallback) {

	m_jsonValidationFailureCallback = jsonValidationFailureCallback;
}

void configWorker::invokeWriteCallback(bool writeStatus) {

	if(m_writeCallback) {

		m_writeCallback(writeStatus);
	}		
}

void configWorker::invokeReadCallback(bool readStatus) {

	if(m_readCallback) {

		m_readCallback(readStatus);
	}		
}

void configWorker::invokeJsonValidationFailureCallback() {

	if(m_jsonValidationFailureCallback) {

		m_jsonValidationFailureCallback();
	}
}

bool configWorker::validateFileAccess() {

	if (!fs::exists(m_configPath) || (!fs::is_regular_file(m_configPath))) {
		
		return false;
	}
	
	return true;
}

void configWorker::readConfig() {

	/***********************************************************************************************************
	 * 
	 *  Если в файловой системе отсутствует конфиг с данными по пулям и винтовкам с другой сопутствующей инфой, 
	 *  то надо инициировать все дефолтными значениями и сохранить в новый конфиг-файл  
	 * 
	 * ********************************************************************************************************/

	if (!validateFileAccess()) {
		
		setSettingsToDefaults();
		saveConfig();
		return;
	}


	/***********************************************************************************************************
	 * 
	 *  Если же конфиг присутствует, то надо прочитать его, проверить валидный ли JSON и если не валидный, то
	 * 	выйти, в противном случае -> десерилизовать в структуры, хранящие около баллистические данные
	 * 
	 * ********************************************************************************************************/

	m_file.open(m_configPath.string(), std::ios::in);

	bool readStatus = m_file.is_open();

	if(!readStatus) {
		
		invokeReadCallback(readStatus);
		return;
	}

	try {
		
		parseContent();
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

void configWorker::parseContent() {
	
	auto bodyJson = nlohmann::json::parse(m_file);
	m_file.close();

	/*************** Parsing for bullets data array ***************/

	m_bullets.indexOfSelected	=	bodyJson["Bullets"]["index"].get<uint8_t>();

	uint8_t i = 0;
	for(const auto& bullet : bodyJson["Bullets"]["bullet_list"]) {

		m_bullets.bullet[i].name		=  bullet["name"].get<std::string>();
		m_bullets.bullet[i].DF			=  m_DFs.at(bullet["DF"].get<std::string>());
		m_bullets.bullet[i].BC			=  bullet["BC"].get<float>();
		m_bullets.bullet[i].MV			=  bullet["V0"].get<uint16_t>();
		m_bullets.bullet[i].length		=  bullet["lenght"].get<float>();
		m_bullets.bullet[i].weight		=  bullet["weight"].get<uint16_t>();
		m_bullets.bullet[i].caliber		=  bullet["diam."].get<float>();
		m_bullets.bullet[i].MV_temp		=  bullet["V0temp"].get<int8_t>();
		m_bullets.bullet[i].thermSens	=  bullet["therm"].get<float>();
		m_bullets.bullet[i].CF_M0_9		=  bullet["CCF_0.9"].get<float>();
		m_bullets.bullet[i].CF_M1_0		=  bullet["CCF_1.0"].get<float>();
		m_bullets.bullet[i].CF_M1_1		=  bullet["CCF_1.1"].get<float>();
	
		i++;

		if(i == MAX_BULLETS_QUANTITY) {
			break;
		}
	}

	/*************** Parsing for rifle data array ***************/

	m_rifles.indexOfSelected	=	bodyJson["Rifles"]["index"].get<uint8_t>();

	i = 0;
	for(const auto& rifle : bodyJson["Rifles"]["rifle_list"]) {

		m_rifles.rifle[i].name				=  rifle["name"].get<std::string>();	
		m_rifles.rifle[i].zero_dist			=  rifle["zero"].get<uint16_t>();
		m_rifles.rifle[i].scope_hight		=  rifle["scope_height"].get<float>();
		m_rifles.rifle[i].twist				=  rifle["twist"].get<float>();
		m_rifles.rifle[i].twist_dir			=  rifle["twist.dir"].get<std::string>() == "R" ? RIGHT_TWIST : LEFT_TWIST;
		m_rifles.rifle[i].zeroing			=  rifle["zero.atm"].get<std::string>() == "here" ? HERE : NOT_HERE;
		m_rifles.rifle[i].zero_T			=  rifle["zero.temp"].get<int8_t>();	
		m_rifles.rifle[i].zero_P			=  rifle["zero.press"].get<uint16_t>();
		m_rifles.rifle[i].vert_drift		=  rifle["POI_vert"].get<float>();
		m_rifles.rifle[i].vert_drift_dir	=  rifle["POI_vert_dir"].get<std::string>() == "up" ? POI_UP : POI_DOWN;
		m_rifles.rifle[i].horiz_drift		=  rifle["POI_horiz"].get<float>();
		m_rifles.rifle[i].horiz_drift_dir	=  rifle["POI_horiz_dir"].get<std::string>() == "left" ? POI_LEFT : POI_RIGHT;
		m_rifles.rifle[i].scope_units		=  rifle["units"].get<std::string>() == "MRAD" ? MRAD_UNITS : MOA_UNITS;
		m_rifles.rifle[i].vert_click		=  rifle["vert.click"].get<float>();
		m_rifles.rifle[i].horiz_click		=  rifle["horiz.click"].get<float>();

		i++;
		
		if(i == MAX_RIFLES_QUANTITY) {
			break;
		}
	}

	/*************** Parsing for location data ***************/

	m_settings.latitude	= bodyJson["Location"]["latitude"].get<float>();

	/*************** Parsing for target data ***************/

	m_target.distance		= bodyJson["Target"]["distance"].get<uint16_t>();
	m_target.terrainAngle	= bodyJson["Target"]["angle"].get<uint8_t>();
	m_target.speedMILs		= bodyJson["Target"]["speed_MILs"].get<float>();
	m_target.azimuth		= bodyJson["Target"]["azimuth"].get<uint16_t>();
	m_target.windAngle		= bodyJson["Target"]["wind_angle"].get<uint16_t>();

	/*************** Parsing for inputs data ***************/

	m_inputs.koriolis	= bodyJson["Inputs"]["koriolis"].get<bool>();
	m_inputs.termoCorr	= bodyJson["Inputs"]["thermal"].get<bool>();
	m_inputs.rangeCard	= bodyJson["Inputs"]["range_card"].get<bool>();
	m_inputs.aeroJump	= bodyJson["Inputs"]["aero_jump"].get<bool>();

	/*************** Parsing for mildot calculator data ***************/

	m_mildot.sizeMeters	= bodyJson["Mildot"]["size"].get<float>();
	m_mildot.sizeMils	= bodyJson["Mildot"]["mils"].get<float>();
}

ballistics::bullets_info_t& configWorker::getBulletsInfo() {

	return m_bullets;
}

ballistics::bullet_t& configWorker::getCurrentBulletInfo() {

	return m_bullets.bullet[m_bullets.indexOfSelected];
}

ballistics::rifles_info_t& configWorker::getRiflesInfo() {

	return m_rifles;
}

ballistics::rifle_t& configWorker::getCurrentRifleInfo() {

	return m_rifles.rifle[m_rifles.indexOfSelected];
}

the_device::settings_t& configWorker::getDeviceSettings() {

	return m_settings;
}

BC::inputs_t& configWorker::getBCInputs() {

	return m_inputs;
}

BC::target_info_t& configWorker::getTargetData() {

	return m_target;
}

BC::mildot_inputs_t& configWorker::getMildotInputs() {

	return m_mildot;
}

void configWorker::setSettingsToDefaults() {

	m_bullets.indexOfSelected =	BALLISTICS_DEFAULT_BULLET_INDEX;

	for(uint8_t i = 0; i < MAX_BULLETS_QUANTITY; i++) {

		m_bullets.bullet[i].name		=  std::string(BALLISTICS_DEFAULT_BULLET_PREFIX) + std::to_string(i + 1);
		m_bullets.bullet[i].DF			=  BALLISTICS_DEFAULT_BULLET_DF;
		m_bullets.bullet[i].BC			=  BALLISTICS_DEFAULT_BULLET_BC;
		m_bullets.bullet[i].MV			=  BALLISTICS_DEFAULT_BULLET_MV;
		m_bullets.bullet[i].length		=  BALLISTICS_DEFAULT_BULLET_LENGHT;
		m_bullets.bullet[i].weight		=  BALLISTICS_DEFAULT_BULLET_WEIGHT;
		m_bullets.bullet[i].caliber		=  BALLISTICS_DEFAULT_BULLET_DIAM;
		m_bullets.bullet[i].MV_temp		=  BALLISTICS_DEFAULT_BULLET_MV_TEMP;
		m_bullets.bullet[i].thermSens	=  BALLISTICS_DEFAULT_BULLET_THERMAL;
		m_bullets.bullet[i].CF_M0_9		=  BALLISTICS_DEFAULT_BULLET_CFACTOR;
		m_bullets.bullet[i].CF_M1_0		=  BALLISTICS_DEFAULT_BULLET_CFACTOR;
		m_bullets.bullet[i].CF_M1_1		=  BALLISTICS_DEFAULT_BULLET_CFACTOR;
	}

	m_rifles.indexOfSelected = BALLISTICS_DEFAULT_RIFLE_INDEX;

	for(uint8_t i = 0; i < MAX_RIFLES_QUANTITY; i++) {

		m_rifles.rifle[i].name				=  std::string(BALLISTICS_DEFAULT_RIFLE_PREFIX) + std::to_string(i + 1);
		m_rifles.rifle[i].zero_dist			=  BALLISTICS_DEFAULT_RIFLE_ZERO_AT;	
		m_rifles.rifle[i].scope_hight		=  BALLISTICS_DEFAULT_RIFLE_SCOPEHIGHT;
		m_rifles.rifle[i].twist				=  BALLISTICS_DEFAULT_RIFLE_TWIST;
		m_rifles.rifle[i].twist_dir			=  BALLISTICS_DEFAULT_RIFLE_TWIST_DIR;
		m_rifles.rifle[i].zeroing			=  BALLISTICS_DEFAULT_RIFLE_ZEROING;
		m_rifles.rifle[i].zero_T			=  BALLISTICS_DEFAULT_RIFLE_TEMP;	
		m_rifles.rifle[i].zero_P			=  BALLISTICS_DEFAULT_RIFLE_PRESS;	
		m_rifles.rifle[i].vert_drift		=  BALLISTICS_DEFAULT_RIFLE_VERT_DRIFT;
		m_rifles.rifle[i].vert_drift_dir	=  BALLISTICS_DEFAULT_RIFLE_VERT_DR_DIR;	
		m_rifles.rifle[i].horiz_drift		=  BALLISTICS_DEFAULT_RIFLE_HORIZ_DRIFT;
		m_rifles.rifle[i].horiz_drift_dir	=  BALLISTICS_DEFAULT_RIFLE_HORIZ_DR_DIR;
		m_rifles.rifle[i].scope_units		=  BALLISTICS_DEFAULT_RIFLE_SCOPE_UNITS;
		m_rifles.rifle[i].vert_click		=  BALLISTICS_DEFAULT_RIFLE_VERT_CLICK;
		m_rifles.rifle[i].horiz_click		=  BALLISTICS_DEFAULT_RIFLE_HORIZ_CLICK;
	}

	m_settings.latitude 	= BALLISTICS_DEFAULT_LATITUDE;

	m_target.distance		= BALLISTICS_DEFAULT_DISTANCE;
	m_target.terrainAngle	= BALLISTICS_DEFAULT_ANGLE;
	m_target.speedMILs		= BALLISTICS_DEFAULT_SPEED_MILLS;
	m_target.azimuth		= BALLISTICS_DEFAULT_AZIMUTH;
	m_target.windAngle		= BALLISTICS_DEFAULT_WIND_ANGLE;

	m_inputs.koriolis		= BALLISTICS_DEFAULT_KORIOLIS_OPTION;
	m_inputs.termoCorr		= BALLISTICS_DEFAULT_THERMAL_OPTION;
	m_inputs.rangeCard		= BALLISTICS_DEFAULT_RANGECARD_OPTION;
	m_inputs.aeroJump		= BALLISTICS_DEFAULT_AEROJUMP_OPTION;

	m_mildot.sizeMeters		= BALLISTICS_DEFAULT_MILDOT_SIZE;
	m_mildot.sizeMils		= BALLISTICS_DEFAULT_MILDOT_MILS;
}

void configWorker::saveConfig() {

	m_file.open(m_configPath.string(), std::ios::out | std::ios::trunc);

	bool writeStatus = m_file.is_open();

	if(!writeStatus) {
		invokeWriteCallback(writeStatus);
		return;
	}

	contentToJson();

	m_file << std::fixed << std::setprecision(3) << m_jsonString;
	m_file.close();
	invokeWriteCallback(writeStatus);
}

void configWorker::contentToJson() {

	m_jsonString.clear();
	m_jsonString.reserve(BALLISTICS_JSON_BUFFER_SIZE);

	nlohmann::json responceJson;

	responceJson["Location"]["latitude"] 	= m_settings.latitude;

	responceJson["Target"]["distance"]		= m_target.distance;
	responceJson["Target"]["angle"]			= m_target.terrainAngle;
	responceJson["Target"]["speed_MILs"]	= m_target.speedMILs;
	responceJson["Target"]["azimuth"]		= m_target.azimuth;
	responceJson["Target"]["wind_angle"]	= m_target.windAngle;

	responceJson["Inputs"]["koriolis"]		= m_inputs.koriolis;
	responceJson["Inputs"]["thermal"]		= m_inputs.termoCorr;
	responceJson["Inputs"]["range_card"]	= m_inputs.rangeCard;
	responceJson["Inputs"]["aero_jump"]		= m_inputs.aeroJump;

	responceJson["Mildot"]["size"]			= m_mildot.sizeMeters;
	responceJson["Mildot"]["mils"]			= m_mildot.sizeMils;

	responceJson["Bullets"]["index"]		= m_bullets.indexOfSelected;
	responceJson["Rifles"]["index"]			= m_rifles.indexOfSelected;

	for(uint8_t i = 0; i < MAX_RIFLES_QUANTITY; i++) {

		nlohmann::json bullet;

		bullet["name"] = m_bullets.bullet[i].name;
		bullet["DF"] = getDFNameByDFType(m_bullets.bullet[i].DF);
		bullet["BC"] = m_bullets.bullet[i].BC;
		bullet["CDM"] = nlohmann::json::object();
		bullet["MBC"] = nlohmann::json::object();
		bullet["V0"] = m_bullets.bullet[i].MV;
		bullet["lenght"] = m_bullets.bullet[i].length;
		bullet["weight"] = m_bullets.bullet[i].weight;
		bullet["diam."] = m_bullets.bullet[i].caliber;
		bullet["CCF_0.9"] = m_bullets.bullet[i].CF_M0_9;
		bullet["CCF_1.0"] = m_bullets.bullet[i].CF_M1_0;
		bullet["CCF_1.1"] = m_bullets.bullet[i].CF_M1_1;
		bullet["V0temp"] = m_bullets.bullet[i].MV_temp;
		bullet["therm"] = m_bullets.bullet[i].thermSens;

		responceJson["Bullets"]["bullet_list"].push_back(bullet);
	}

	for(uint8_t i = 0; i < MAX_RIFLES_QUANTITY; i++) {

		nlohmann::json rifle;

		rifle["name"] = m_rifles.rifle[i].name;

		rifle["zero"] = m_rifles.rifle[i].zero_dist;
		rifle["scope_height"] = m_rifles.rifle[i].scope_hight;
		rifle["twist"] = m_rifles.rifle[i].twist;
		rifle["twist.dir"] = (m_rifles.rifle[i].twist_dir == RIGHT_TWIST) ? "R" : "L";
		rifle["zero.atm"] = (m_rifles.rifle[i].zeroing == HERE) ? "here" : "not here";		
		rifle["zero.temp"] = m_rifles.rifle[i].zero_T;
		rifle["zero.press"] = m_rifles.rifle[i].zero_P;
		rifle["POI_vert"] = m_rifles.rifle[i].vert_drift;
		rifle["POI_vert_dir"] = (m_rifles.rifle[i].vert_drift_dir == POI_UP) ? "up" : "down";
		rifle["POI_horiz"] = m_rifles.rifle[i].horiz_drift;
		rifle["POI_horiz_dir"] = (m_rifles.rifle[i].horiz_drift_dir == POI_LEFT) ? "left" : "right";
		rifle["units"] = (m_rifles.rifle[i].scope_units == MRAD_UNITS) ? "MRAD" : "MOA";
		rifle["vert.click"] = m_rifles.rifle[i].vert_click;
		rifle["horiz.click"] = m_rifles.rifle[i].horiz_click;

		responceJson["Rifles"]["rifle_list"].push_back(rifle);
	}

	m_jsonString = responceJson.dump(4);
}

const std::string configWorker::getDFNameByDFType(uint8_t DFtype) {

	for (const auto& DFPair : m_DFs) {
		
		if (DFPair.second == DFtype) {
			
			return DFPair.first;
		}
	}

	return "N/A";
}