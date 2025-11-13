#include <string>

///////////////////////////////////////////////////////////////////////////////

#include  "profiles_config_worker.h"
#include "nlohmann.h"

///////////////////////////////////////////////////////////////////////////////

UserProfilesWorker::UserProfilesWorker(const char* configPath) : m_configPath(std::string(configPath)) {}

bool UserProfilesWorker::validateFileAccess() {

	if (!fs::exists(m_configPath) || (!fs::is_regular_file(m_configPath))) {
		
		return false;
	}
	
	return true;
}

void UserProfilesWorker::invokeWriteCallback(bool writeStatus) {

	if(m_writeCallback) {
		m_writeCallback(writeStatus);
	}		
}

void UserProfilesWorker::invokeReadCallback(bool validationStatus) {

	if(m_readCallback) {
		m_readCallback(validationStatus);
	}		
}

void UserProfilesWorker::invokeJsonValidationFailureCallback() {

	if(m_jsonValidationFailureCallback) {
		m_jsonValidationFailureCallback();
	}
}

void UserProfilesWorker::setConfigWriteCallback(std::function<void(bool)> writeCallback) {
	
	m_writeCallback = writeCallback;
}

void UserProfilesWorker::setConfigReadCallback(std::function<void(bool)> readCallback) {
	
	m_readCallback = readCallback;
}

void UserProfilesWorker::setJsonValidationCallback(std::function<void()> jsonValidationFailureCallback) {
	
	m_jsonValidationFailureCallback = jsonValidationFailureCallback;
}	

user::TP_profile_info_t& UserProfilesWorker::getTPprofiles() {

	return m_TPprofiles;
}

user::TP_common_settings_t& UserProfilesWorker::getTPCommons() {

	return m_TPcommons;
}

user::T_common_settings_HW_t UserProfilesWorker::getTPHWcommons() {

	return m_TPHWcommons;
}

///////////////////////////////////////////////////////////////////////////////

void UserProfilesWorker::loadFromFile() {

	/***********************************************************************************************************
	 * 
	 *  Если в файловой системе отсутствует конфиг с пользовательскими настройками, то надо инициировать все 
	 *  дефолтными значениями и сохранить новый конфиг-файл  
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
	 * 	выйти, в противном случае -> десерилизовать в соответствующие структуры
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

		/************* Parsing for profiles *************/

		m_TPprofiles.indexOfSelected = bodyJson["Profiles"]["index"].get<decltype(user::TPprofiles::indexOfSelected)>();

		uint8_t i = 0;
		for(const auto& profile : bodyJson["Profiles"]["profiles_list"]) {

			m_TPprofiles.profile[i].name = profile["name"].get<std::string>();
			m_TPprofiles.profile[i].correctionX 			= profile["correctionX"].get<decltype(user::TPprofile::correctionX)>();
			m_TPprofiles.profile[i].correctionY 			= profile["correctionY"].get<decltype(user::TPprofile::correctionY)>();
			m_TPprofiles.profile[i].correctionH 			= profile["correctionH"].get<decltype(user::TPprofile::correctionH)>();
			m_TPprofiles.profile[i].correctionV 			= profile["correctionV"].get<decltype(user::TPprofile::correctionV)>();
			m_TPprofiles.profile[i].brightnessOLED 			= profile["brightnessOLED"].get<decltype(user::TPprofile::brightnessOLED)>();
			m_TPprofiles.profile[i].contrastOLED 			= profile["contrastOLED"].get<decltype(user::TPprofile::contrastOLED)>();
			m_TPprofiles.profile[i].zoomTF 					= profile["zoomTF"].get<decltype(user::TPprofile::zoomTF)>();
			m_TPprofiles.profile[i].pictureProfileIndex		= profile["pictureProfileIndex"].get<decltype(user::TPprofile::pictureProfileIndex)>();
			m_TPprofiles.profile[i].contrastPredust 		= profile["contrastPredust"].get<decltype(user::TPprofile::contrastPredust)>();
			m_TPprofiles.profile[i].gammaPredust 			= profile["gammaPredust"].get<decltype(user::TPprofile::gammaPredust)>();
			m_TPprofiles.profile[i].palette 				= profile["palette"].get<decltype(user::TPprofile::palette)>();
			m_TPprofiles.profile[i].paletteWidth 			= profile["paletteWidth"].get<decltype(user::TPprofile::paletteWidth)>();
			m_TPprofiles.profile[i].paletteOffset 			= profile["paletteOffset"].get<decltype(user::TPprofile::paletteOffset)>();
			m_TPprofiles.profile[i].pricelType 				= profile["pricelType"].get<decltype(user::TPprofile::pricelType)>();
			m_TPprofiles.profile[i].pricelColor 			= profile["pricelColor"].get<decltype(user::TPprofile::pricelColor)>();
			m_TPprofiles.profile[i].pricelBright 			= profile["pricelBright"].get<decltype(user::TPprofile::pricelBright)>();

			i++;

			if(i == USER_PROFILES_NUMBER_MAX) {
				break;
			}
		}

		m_TPcommons.Menu_Main_Number 			= bodyJson["TPCommons"]["Menu_Main_Number"].get<decltype(user::TPCommonSettings::Menu_Main_Number)>();
		m_TPcommons.Menu_TimeOut 				= bodyJson["TPCommons"]["Menu_TimeOut"].get<decltype(user::TPCommonSettings::Menu_TimeOut)>();
		m_TPcommons.Lang 						= bodyJson["TPCommons"]["Lang"].get<decltype(user::TPCommonSettings::Lang)>();
		m_TPcommons.OLED_shift_x 				= bodyJson["TPCommons"]["OLED_shift_x"].get<decltype(user::TPCommonSettings::OLED_shift_x)>();
		m_TPcommons.OLED_shift_y 				= bodyJson["TPCommons"]["OLED_shift_y"].get<decltype(user::TPCommonSettings::OLED_shift_y)>();
		m_TPcommons.OLED_shift_dir_x 			= bodyJson["TPCommons"]["OLED_shift_dir_x"].get<decltype(user::TPCommonSettings::OLED_shift_dir_x)>();
		m_TPcommons.OLED_shift_dir_y 			= bodyJson["TPCommons"]["OLED_shift_dir_y"].get<decltype(user::TPCommonSettings::OLED_shift_dir_y)>();
		m_TPcommons.Dalnomer_TargetSize 		= bodyJson["TPCommons"]["Dalnomer_TargetSize"].get<decltype(user::TPCommonSettings::Dalnomer_TargetSize)>();
		m_TPcommons.Dalnomer_TargetSizeIndex 	= bodyJson["TPCommons"]["Dalnomer_TargetSizeIndex"].get<decltype(user::TPCommonSettings::Dalnomer_TargetSizeIndex)>();
		m_TPcommons.Dalnomer_StepSize 			= bodyJson["TPCommons"]["Dalnomer_StepSize"].get<decltype(user::TPCommonSettings::Dalnomer_StepSize)>();
		m_TPcommons.Dalnomer_BoxSize 			= bodyJson["TPCommons"]["Dalnomer_BoxSize"].get<decltype(user::TPCommonSettings::Dalnomer_BoxSize)>();
		m_TPcommons.Dalnomer_BoxSize_Last 		= bodyJson["TPCommons"]["Dalnomer_BoxSize_Last"].get<decltype(user::TPCommonSettings::Dalnomer_BoxSize_Last)>();
		m_TPcommons.Dalnomer_Distance 			= bodyJson["TPCommons"]["Dalnomer_Distance"].get<decltype(user::TPCommonSettings::Dalnomer_Distance)>();
		m_TPcommons.Dalnomer_OnAim 				= bodyJson["TPCommons"]["Dalnomer_OnAim"].get<decltype(user::TPCommonSettings::Dalnomer_OnAim)>();
		m_TPcommons.BadPixThreshold 			= bodyJson["TPCommons"]["BadPixThreshold"].get<decltype(user::TPCommonSettings::BadPixThreshold)>();
		m_TPcommons.AimFlashCenter 				= bodyJson["TPCommons"]["AimFlashCenter"].get<decltype(user::TPCommonSettings::AimFlashCenter)>();
		m_TPcommons.ZoomInNM 					= bodyJson["TPCommons"]["ZoomInNM"].get<decltype(user::TPCommonSettings::ZoomInNM)>();
		m_TPcommons.ChangeTPInNM 				= bodyJson["TPCommons"]["ChangeTPInNM"].get<decltype(user::TPCommonSettings::ChangeTPInNM)>();
		m_TPcommons.INVencL 					= bodyJson["TPCommons"]["INVencL"].get<decltype(user::TPCommonSettings::INVencL)>();
		m_TPcommons.INVencT 					= bodyJson["TPCommons"]["INVencT"].get<decltype(user::TPCommonSettings::INVencT)>();
		m_TPcommons.INVencR 					= bodyJson["TPCommons"]["INVencR"].get<decltype(user::TPCommonSettings::INVencR)>();
		m_TPcommons.ZoomStep 					= bodyJson["TPCommons"]["ZoomStep"].get<decltype(user::TPCommonSettings::ZoomStep)>();

		m_TPHWcommons.DisplayModel 				= bodyJson["TPHWcommons"]["DisplayModel"].get<decltype(user::TPCommonSettingsHW::DisplayModel)>();
		m_TPHWcommons.DisplayRes 				= bodyJson["TPHWcommons"]["DisplayRes"].get<decltype(user::TPCommonSettingsHW::DisplayRes)>();
		m_TPHWcommons.ModuleType 				= bodyJson["TPHWcommons"]["ModuleType"].get<decltype(user::TPCommonSettingsHW::ModuleType)>();
		m_TPHWcommons.Module_ver 				= bodyJson["TPHWcommons"]["Module_ver"].get<decltype(user::TPCommonSettingsHW::Module_ver)>();
		m_TPHWcommons.PixelStep 				= bodyJson["TPHWcommons"]["PixelStep"].get<decltype(user::TPCommonSettingsHW::PixelStep)>();
		m_TPHWcommons.fAutoDetectedOLEDModel 	= bodyJson["TPHWcommons"]["fAutoDetectedOLEDModel"].get<decltype(user::TPCommonSettingsHW::fAutoDetectedOLEDModel)>();
		m_TPHWcommons.fAutoDetectedOLEDRes 		= bodyJson["TPHWcommons"]["fAutoDetectedOLEDRes"].get<decltype(user::TPCommonSettingsHW::fAutoDetectedOLEDRes)>();
		m_TPHWcommons.fAutoDetectedModule 		= bodyJson["TPHWcommons"]["fAutoDetectedModule"].get<decltype(user::TPCommonSettingsHW::fAutoDetectedModule)>();
		m_TPHWcommons.fAutoDetectedModuleVer 	= bodyJson["TPHWcommons"]["fAutoDetectedModuleVer"].get<decltype(user::TPCommonSettingsHW::fAutoDetectedModuleVer)>();
		m_TPHWcommons.FocalLenght 				= bodyJson["TPHWcommons"]["FocalLenght"].get<decltype(user::TPCommonSettingsHW::FocalLenght)>();
		m_TPHWcommons.PixelSize 				= bodyJson["TPHWcommons"]["PixelSize"].get<decltype(user::TPCommonSettingsHW::PixelSize)>();
		m_TPHWcommons.ScaleSensorOLED 			= bodyJson["TPHWcommons"]["ScaleSensorOLED"].get<decltype(user::TPCommonSettingsHW::ScaleSensorOLED)>();
		m_TPHWcommons.ADC_BatteryCorr 			= bodyJson["TPHWcommons"]["ADC_BatteryCorr"].get<decltype(user::TPCommonSettingsHW::ADC_BatteryCorr)>();
		m_TPHWcommons.ADC_TemperatureCorr 		= bodyJson["TPHWcommons"]["ADC_TemperatureCorr"].get<decltype(user::TPCommonSettingsHW::ADC_TemperatureCorr)>();
		m_TPHWcommons.MountAngle 				= bodyJson["TPHWcommons"]["MountAngle"].get<decltype(user::TPCommonSettingsHW::MountAngle)>();
		m_TPHWcommons.DisplayResX 				= bodyJson["TPHWcommons"]["DisplayResX"].get<decltype(user::TPCommonSettingsHW::DisplayResX)>();
		m_TPHWcommons.DisplayResY 				= bodyJson["TPHWcommons"]["DisplayResY"].get<decltype(user::TPCommonSettingsHW::DisplayResY)>();
		m_TPHWcommons.DisplayResX2 				= bodyJson["TPHWcommons"]["DisplayResX2"].get<decltype(user::TPCommonSettingsHW::DisplayResX2)>();
		m_TPHWcommons.DisplayResY2 				= bodyJson["TPHWcommons"]["DisplayResY2"].get<decltype(user::TPCommonSettingsHW::DisplayResY2)>();
		m_TPHWcommons.ModuleResX 				= bodyJson["TPHWcommons"]["ModuleResX"].get<decltype(user::TPCommonSettingsHW::ModuleResX)>();
		m_TPHWcommons.ModuleResY 				= bodyJson["TPHWcommons"]["ModuleResY"].get<decltype(user::TPCommonSettingsHW::ModuleResY)>();
		m_TPHWcommons.ModuleResX2 				= bodyJson["TPHWcommons"]["ModuleResX2"].get<decltype(user::TPCommonSettingsHW::ModuleResX2)>();
		m_TPHWcommons.ModuleResY2 				= bodyJson["TPHWcommons"]["ModuleResY2"].get<decltype(user::TPCommonSettingsHW::ModuleResY2)>();

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

void UserProfilesWorker::saveToFile() {

	m_file.open(m_configPath.string(), std::ios::out | std::ios::trunc);
	
	bool writeStatus = m_file.is_open();

	if(!writeStatus) {
		invokeWriteCallback(writeStatus);
		return;
	}

	nlohmann::json responceJson;

	responceJson["Profiles"]["index"] = m_TPprofiles.indexOfSelected;

	for(uint8_t i = 0; i < USER_PROFILES_NUMBER_MAX; i++) {

		nlohmann::json profile;

		profile["name"] 				= m_TPprofiles.profile[i].name;
		profile["correctionX"] 			= m_TPprofiles.profile[i].correctionX;
		profile["correctionY"] 			= m_TPprofiles.profile[i].correctionY;
		profile["correctionH"] 			= m_TPprofiles.profile[i].correctionH;
		profile["correctionV"] 			= m_TPprofiles.profile[i].correctionV;
		profile["brightnessOLED"] 		= m_TPprofiles.profile[i].brightnessOLED;
		profile["contrastOLED"] 		= m_TPprofiles.profile[i].contrastOLED;
		profile["zoomTF"] 				= m_TPprofiles.profile[i].zoomTF;
		profile["pictureProfileIndex"]	= m_TPprofiles.profile[i].pictureProfileIndex;
		profile["contrastPredust"] 		= m_TPprofiles.profile[i].contrastPredust;
		profile["gammaPredust"]			= m_TPprofiles.profile[i].gammaPredust;
		profile["palette"] 				= m_TPprofiles.profile[i].palette;
		profile["paletteWidth"] 		= m_TPprofiles.profile[i].paletteWidth;
		profile["paletteOffset"] 		= m_TPprofiles.profile[i].paletteOffset;
		profile["pricelType"] 			= m_TPprofiles.profile[i].pricelType;
		profile["pricelColor"] 			= m_TPprofiles.profile[i].pricelColor;
		profile["pricelBright"] 		= m_TPprofiles.profile[i].pricelBright;

		responceJson["Profiles"]["profiles_list"].push_back(profile);
	}

	responceJson["TPCommons"]["Menu_Main_Number"]			= m_TPcommons.Menu_Main_Number;
	responceJson["TPCommons"]["Menu_TimeOut"]				= m_TPcommons.Menu_TimeOut;
	responceJson["TPCommons"]["Lang"]						= m_TPcommons.Lang;
	responceJson["TPCommons"]["OLED_shift_x"]				= m_TPcommons.OLED_shift_x;
	responceJson["TPCommons"]["OLED_shift_y"]				= m_TPcommons.OLED_shift_y;
	responceJson["TPCommons"]["OLED_shift_dir_x"]			= m_TPcommons.OLED_shift_dir_x;
	responceJson["TPCommons"]["OLED_shift_dir_y"]			= m_TPcommons.OLED_shift_dir_y;
	responceJson["TPCommons"]["Dalnomer_TargetSize"]		= m_TPcommons.Dalnomer_TargetSize;
	responceJson["TPCommons"]["Dalnomer_TargetSizeIndex"]	= m_TPcommons.Dalnomer_TargetSizeIndex;
	responceJson["TPCommons"]["Dalnomer_StepSize"]			= m_TPcommons.Dalnomer_StepSize;
	responceJson["TPCommons"]["Dalnomer_BoxSize"]			= m_TPcommons.Dalnomer_BoxSize ;
	responceJson["TPCommons"]["Dalnomer_BoxSize_Last"]		= m_TPcommons.Dalnomer_BoxSize_Last;
	responceJson["TPCommons"]["Dalnomer_Distance"]			= m_TPcommons.Dalnomer_Distance;
	responceJson["TPCommons"]["Dalnomer_OnAim"]				= m_TPcommons.Dalnomer_OnAim;
	responceJson["TPCommons"]["BadPixThreshold"]			= m_TPcommons.BadPixThreshold;
	responceJson["TPCommons"]["AimFlashCenter"]				= m_TPcommons.AimFlashCenter;
	responceJson["TPCommons"]["ZoomInNM"]					= m_TPcommons.ZoomInNM;
	responceJson["TPCommons"]["ChangeTPInNM"]				= m_TPcommons.ChangeTPInNM;
	responceJson["TPCommons"]["INVencL"]					= m_TPcommons.INVencL;
	responceJson["TPCommons"]["INVencT"]					= m_TPcommons.INVencT;
	responceJson["TPCommons"]["INVencR"]					= m_TPcommons.INVencR;
	responceJson["TPCommons"]["ZoomStep"]					= m_TPcommons.ZoomStep;

	responceJson["TPHWcommons"]["DisplayModel"]				= m_TPHWcommons.DisplayModel;
	responceJson["TPHWcommons"]["DisplayRes"]				= m_TPHWcommons.DisplayRes;
	responceJson["TPHWcommons"]["ModuleType"]				= m_TPHWcommons.ModuleType;
	responceJson["TPHWcommons"]["Module_ver"]				= m_TPHWcommons.Module_ver;
	responceJson["TPHWcommons"]["PixelStep"]				= m_TPHWcommons.PixelStep;
	responceJson["TPHWcommons"]["fAutoDetectedOLEDModel"]	= m_TPHWcommons.fAutoDetectedOLEDModel;
	responceJson["TPHWcommons"]["fAutoDetectedOLEDRes"]		= m_TPHWcommons.fAutoDetectedOLEDRes;
	responceJson["TPHWcommons"]["fAutoDetectedModule"]		= m_TPHWcommons.fAutoDetectedModule;
	responceJson["TPHWcommons"]["fAutoDetectedModuleVer"]	= m_TPHWcommons.fAutoDetectedModuleVer;
	responceJson["TPHWcommons"]["FocalLenght"]				= m_TPHWcommons.FocalLenght;
	responceJson["TPHWcommons"]["PixelSize"]				= m_TPHWcommons.PixelSize;
	responceJson["TPHWcommons"]["ScaleSensorOLED"]			= m_TPHWcommons.ScaleSensorOLED;
	responceJson["TPHWcommons"]["ADC_BatteryCorr"]			= m_TPHWcommons.ADC_BatteryCorr;
	responceJson["TPHWcommons"]["ADC_TemperatureCorr"]		= m_TPHWcommons.ADC_TemperatureCorr;
	responceJson["TPHWcommons"]["MountAngle"]				= m_TPHWcommons.MountAngle;
	responceJson["TPHWcommons"]["DisplayResX"]				= m_TPHWcommons.DisplayResX;
	responceJson["TPHWcommons"]["DisplayResY"]				= m_TPHWcommons.DisplayResY;
	responceJson["TPHWcommons"]["DisplayResX2"]				= m_TPHWcommons.DisplayResX2;
	responceJson["TPHWcommons"]["DisplayResY2"]				= m_TPHWcommons.DisplayResY2;
	responceJson["TPHWcommons"]["ModuleResX"]				= m_TPHWcommons.ModuleResX;
	responceJson["TPHWcommons"]["ModuleResY"]				= m_TPHWcommons.ModuleResY;
	responceJson["TPHWcommons"]["ModuleResX2"]				= m_TPHWcommons.ModuleResX2;
	responceJson["TPHWcommons"]["ModuleResY2"]				= m_TPHWcommons.ModuleResY2;

	m_file << responceJson.dump(4);
	m_file.close();

	invokeWriteCallback(writeStatus);
}

void UserProfilesWorker::setSettingsToDefaults() {

	m_TPprofiles.indexOfSelected = 0;

	for(uint8_t i = 0; i < USER_PROFILES_NUMBER_MAX; i++) {

		m_TPprofiles.profile[i].name				= std::string(USER_PROFILE_DEFAULT_NAME_PREFIX) + std::to_string(i + 1);
		m_TPprofiles.profile[i].correctionX			= USER_PROFILE_DEFAULT_CORRECTION_X;
		m_TPprofiles.profile[i].correctionY			= USER_PROFILE_DEFAULT_CORRECTION_Y;
		m_TPprofiles.profile[i].correctionH			= USER_PROFILE_DEFAULT_CORRECTION_H;
		m_TPprofiles.profile[i].correctionV			= USER_PROFILE_DEFAULT_CORRECTION_V;
		m_TPprofiles.profile[i].brightnessOLED		= USER_PROFILE_DEFAULT_BRIGHNESS;
		m_TPprofiles.profile[i].contrastOLED		= USER_PROFILE_DEFAULT_CONTRAST;
		m_TPprofiles.profile[i].zoomTF				= USER_PROFILE_DEFAULT_ZOOM;
		m_TPprofiles.profile[i].pictureProfileIndex	= USER_PROFILE_DEFAULT_PROFILE_INDEX;
		m_TPprofiles.profile[i].contrastPredust		= USER_PROFILE_DEFAULT_CONTRAST_PREDUST;
		m_TPprofiles.profile[i].gammaPredust		= USER_PROFILE_DEFAULT_GAMMA_PREDUST;
		m_TPprofiles.profile[i].palette				= USER_PROFILE_DEFAULT_PALETTE;
		m_TPprofiles.profile[i].paletteWidth		= USER_PROFILE_DEFAULT_PALETTE_WIDTH;
		m_TPprofiles.profile[i].paletteOffset		= USER_PROFILE_DEFAULT_PALETTE_OFFSET;
		m_TPprofiles.profile[i].pricelType			= USER_PROFILE_DEFAULT_AIM_TYPE;
		m_TPprofiles.profile[i].pricelColor			= USER_PROFILE_DEFAULT_AIM_COLOR_INDEX;
		m_TPprofiles.profile[i].pricelBright		= USER_PROFILE_DEFAULT_AIM_BRIGHTNESS;
	}

	m_TPcommons.Menu_Main_Number			= USER_TF_PROFILE_DEFAULT_MENU_NUMBER;
	m_TPcommons.Menu_TimeOut				= USER_TF_PROFILE_DEFAULT_MENU_TIMEOUT;
	m_TPcommons.Lang						= USER_TF_PROFILE_DEFAULT_UI_LANGUAGE;
	m_TPcommons.OLED_shift_x				= USER_TF_PROFILE_DEFAULT_OLED_SHIFT_X;
	m_TPcommons.OLED_shift_y				= USER_TF_PROFILE_DEFAULT_OLED_SHIFT_Y;
	m_TPcommons.OLED_shift_dir_x			= USER_TF_PROFILE_DEFAULT_OLED_SHIFT_DX;
	m_TPcommons.OLED_shift_dir_y			= USER_TF_PROFILE_DEFAULT_OLED_SHIFT_DY;
	m_TPcommons.Dalnomer_TargetSize			= USER_TF_PROFILE_DEFAULT_RF_TARGET_SIZE;
	m_TPcommons.Dalnomer_TargetSizeIndex	= USER_TF_PROFILE_DEFAULT_RF_TARGET_INDEX;
	m_TPcommons.Dalnomer_StepSize			= USER_TF_PROFILE_DEFAULT_RF_TARGET_SIZE_STEP;
	m_TPcommons.Dalnomer_BoxSize 			= USER_TF_PROFILE_DEFAULT_RF_BOX_SIZE;
	m_TPcommons.Dalnomer_BoxSize_Last		= USER_TF_PROFILE_DEFAULT_RF_BOX_SIZE_LAST;
	m_TPcommons.Dalnomer_Distance			= USER_TF_PROFILE_DEFAULT_RF_CALCULATED_DISTANCE;
	m_TPcommons.Dalnomer_OnAim				= USER_TF_PROFILE_DEFAULT_RF_ON_AIM;
	m_TPcommons.BadPixThreshold				= USER_TF_PROFILE_DEFAULT_BAD_PIXS_THRESHOLD;
	m_TPcommons.AimFlashCenter				= USER_TF_PROFILE_DEFAULT_AIM_FLASHING;
	m_TPcommons.ZoomInNM					= USER_TF_PROFILE_DEFAULT_ZOOM_IN_NM;
	m_TPcommons.ChangeTPInNM				= USER_TF_PROFILE_DEFAULT_TP_CHANGE_IN_NM;
	m_TPcommons.INVencL						= USER_TF_PROFILE_DEFAULT_LEFT_ENC_INVERSION;
	m_TPcommons.INVencT						= USER_TF_PROFILE_DEFAULT_TOP_ENC_INVERSION;
	m_TPcommons.INVencR						= USER_TF_PROFILE_DEFAULT_RIGHT_ENC_INVERSION;
	m_TPcommons.ZoomStep					= USER_TF_PROFILE_DEFAULT_ZOOM_STEP_x10;

	m_TPHWcommons.DisplayModel 				= USER_TF_HW_PROFILE_DEFAULT_DISPLAY_MODEL;
	m_TPHWcommons.DisplayRes 				= USER_TF_HW_PROFILE_DEFAULT_DISPLAY_RESOLUTION;
	m_TPHWcommons.ModuleType 				= USER_TF_HW_PROFILE_DEFAULT_MODULE_TYPE;
	m_TPHWcommons.Module_ver 				= USER_TF_HW_PROFILE_DEFAULT_MODULE_VERSION;
	m_TPHWcommons.PixelStep 				= USER_TF_HW_PROFILE_DEFAULT_PIXEL_STEP;
	m_TPHWcommons.fAutoDetectedOLEDModel 	= USER_TF_HW_PROFILE_DEFAULT_OLED_AUTODETECT;
	m_TPHWcommons.fAutoDetectedOLEDRes 		= USER_TF_HW_PROFILE_DEFAULT_RES_AUTODETECT;
	m_TPHWcommons.fAutoDetectedModule 		= USER_TF_HW_PROFILE_DEFAULT_TF_AUTODETECT;
	m_TPHWcommons.fAutoDetectedModuleVer 	= USER_TF_HW_PROFILE_DEFAULT_TF_VER_AUTODETECT;
	m_TPHWcommons.FocalLenght 				= USER_TF_HW_PROFILE_DEFAULT_FOCAL_LENGHT;
	m_TPHWcommons.PixelSize 				= USER_TF_HW_PROFILE_DEFAULT_PIXEL_SIZE;
	m_TPHWcommons.ScaleSensorOLED 			= USER_TF_HW_PROFILE_DEFAULT_SCALE_OLED;
	m_TPHWcommons.ADC_BatteryCorr 			= USER_TF_HW_PROFILE_DEFAULT_ADC_BATTERY_CORR;
	m_TPHWcommons.ADC_TemperatureCorr 		= USER_TF_HW_PROFILE_DEFAULT_ADC_TEMP_CORR;
	m_TPHWcommons.MountAngle 				= USER_TF_HW_PROFILE_DEFAULT_MOUNT_ANGLE;
	m_TPHWcommons.DisplayResX 				= USER_TF_HW_PROFILE_DEFAULT_OLED_RES_X;
	m_TPHWcommons.DisplayResY 				= USER_TF_HW_PROFILE_DEFAULT_OLED_RES_Y;
	m_TPHWcommons.DisplayResX2 				= USER_TF_HW_PROFILE_DEFAULT_OLED_RES_X2;
	m_TPHWcommons.DisplayResY2 				= USER_TF_HW_PROFILE_DEFAULT_OLED_RES_Y2;
	m_TPHWcommons.ModuleResX 				= USER_TF_HW_PROFILE_DEFAULT_TF_RESX;
	m_TPHWcommons.ModuleResY 				= USER_TF_HW_PROFILE_DEFAULT_TF_RESY;
	m_TPHWcommons.ModuleResX2 				= USER_TF_HW_PROFILE_DEFAULT_TF_RESX2;
	m_TPHWcommons.ModuleResY2 				= USER_TF_HW_PROFILE_DEFAULT_TF_RESY2; 
}
