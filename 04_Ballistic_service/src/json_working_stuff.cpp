#include "json_working_stuff.h"
#include "CFastLog.h"

#include <map>
#include <cstring>
#include <iostream>

static CDMDataArray CDMArray{};
static MBCDataArray MBCArray{};

Bullet s2::datapreparator::parseForBulletData(const nlohmann::json& bodyJson) const {

	/* Parse for bullet data: 
	"Bullet":{"DF":"G7","BC":0.247,"CDM":[{"0.5":0.156},{"0.6":0.456} ...],"V0":740,"lenght":33.15,
	"weight":185,"diam.":7.82,"CCF_0.9":1.015,"CCF_1.0":1.012,"CCF_1.1":1.017,"V0temp":22.5,"therm":1.6} 
	*/

	const std::map<std::string, dragModels> DFStringToEnum {
		{"\"G1\"", G1},
		{"\"G7\"", G7},
		{"\"Gs\"", Gs},
		{"\"CDM\"", CDM},
		{"\"MBCG1\"", MBCG1},
		{"\"MBCG7\"", MBCG7},
	};

	auto DF = DFStringToEnum.at(bodyJson["Bullet"]["DF"].dump());
	auto BC = bodyJson["Bullet"]["BC"].get<double>();
	auto V0 = bodyJson["Bullet"]["V0"].get<uint16_t>();
	auto lenght = bodyJson["Bullet"]["lenght"].get<double>();
	auto weight = bodyJson["Bullet"]["weight"].get<uint16_t>();
	auto dia = bodyJson["Bullet"]["diam."].get<double>();
	auto CCF_09 = bodyJson["Bullet"]["CCF_0.9"].get<double>();
	auto CCF_10 = bodyJson["Bullet"]["CCF_1.0"].get<double>();
	auto CCF_11 = bodyJson["Bullet"]["CCF_1.1"].get<double>();
	auto V0temp = bodyJson["Bullet"]["V0temp"].get<int8_t>();
	auto therm = bodyJson["Bullet"]["therm"].get<double>();

	if(DF == CDM) {

		int i = 0;
		for(const auto& cdPair : bodyJson["Bullet"]["CDM"]) {

			auto Mach = 0.5 + i * 0.1;
			auto Cd = cdPair.get<double>();

			CDMArray[i].MachNumber = Mach;
			CDMArray[i].CD = Cd;

			i++;
		}

		return Bullet{"*", (uint8_t)DF, BC, CCF_09, CCF_10, CCF_11, V0, lenght, weight, dia, V0temp, therm, &CDMArray, USELESS_COMPLEX_DATA};
	}
	else if(DF == MBCG1 || DF == MBCG7) {

		int i = 0;

		for(const auto& bcPair : bodyJson["Bullet"]["MBC"]) {

			auto Mach = 0.5 + i * 0.1;
			auto BCi = bcPair.get<double>();

			MBCArray[i].MachNumber = Mach;
			MBCArray[i].BC = BCi;

			i++;
		}

		return Bullet{"*", (uint8_t)DF, BC, CCF_09, CCF_10, CCF_11, V0, lenght, weight, dia, V0temp, therm, USELESS_COMPLEX_DATA, &MBCArray};
	}
	else {

		return Bullet{"*", (uint8_t)DF, BC, CCF_09, CCF_10, CCF_11, V0, lenght, weight, dia, V0temp, therm, USELESS_COMPLEX_DATA, USELESS_COMPLEX_DATA};
	}

	return Bullet{}; /* Never go there... */
}

Rifle s2::datapreparator::parseForRifleData(const nlohmann::json& bodyJson) const {

	/* Parse for bullet data: 
	"Rifle": {"zero": 100,"scope_height": 8.3,"twist": 254,"twist.dir": "R","zero.atm": "here",
	"zero.temp": -7,"zero.press": 996,"POI_vert": -2.3,"POI_horiz": -1.1,"roll": 7.2}
	*/

	auto zero = bodyJson["Rifle"]["zero"].get<uint16_t>();
	auto scope_height = bodyJson["Rifle"]["scope_height"].get<double>();
	auto twist = bodyJson["Rifle"]["twist"].get<uint16_t>();
	
	auto twistDir = bodyJson["Rifle"]["twist.dir"].dump() == "\"R\"" ? RIGHT_TWIST : LEFT_TWIST;
	auto zeroAthm = bodyJson["Rifle"]["zero.atm"].dump() == "\"here\"" ? HERE : NOT_HERE;

	auto zeroTemp = bodyJson["Rifle"]["zero.temp"].get<int8_t>();
	auto zeroPress = bodyJson["Rifle"]["zero.press"].get<uint16_t>();

	auto vertDrift = bodyJson["Rifle"]["POI_vert"].get<double>();	// - down
	auto horizDrift = bodyJson["Rifle"]["POI_horiz"].get<double>();	// - right

	auto vertDrDir = POI_UP;
	auto horizDrDir = POI_RIGHT;

	if(vertDrift < 0) { // down direction
		
		vertDrift = -vertDrift;
		vertDrDir = POI_DOWN;
	}

	if(horizDrift < 0) { //left direction
		
		horizDrift = -horizDrift;
		horizDrDir = POI_LEFT;
	}

	auto rollAngle = bodyJson["Rifle"]["roll"].get<double>();

	return Rifle{"*", zero, scope_height, (double)twist, (uint8_t)twistDir, (uint8_t)zeroAthm, zeroTemp,
	zeroPress, vertDrift, (uint8_t)vertDrDir, horizDrift, (uint8_t)horizDrDir, (int16_t)rollAngle};
}

Scope s2::datapreparator::parseForScopeData(const nlohmann::json& bodyJson) {

	/* Parse for scope data: 
	"Scope": {"units": "MRAD","vert.click": 0.1,"horiz.click": 0.1} */

	m_unitsIsMrads = bodyJson["Scope"]["units"].dump() == "\"MRAD\"";

	auto angleUnits = m_unitsIsMrads ? MRAD_UNITS : MOA_UNITS;
	auto vertClick = bodyJson["Scope"]["vert.click"].get<double>();
	auto horizClick = bodyJson["Scope"]["horiz.click"].get<double>();

	return Scope{"*", (uint8_t)angleUnits, vertClick, horizClick, MIL_DOT};
}

Options s2::datapreparator::parseForOptions(const nlohmann::json& bodyJson) {

	/* Parse for options data:
	"Options": {"koriolis": true,"rangecard": false,"therm.corr": false,"aerojump": true}
	*/

	auto koriolis = bodyJson["Options"]["koriolis"].get<bool>() ? OPTION_YES : OPTION_NO;
	m_makeRangecard = bodyJson["Options"]["rangecard"].get<bool>();

	auto rangecard = m_makeRangecard ? OPTION_YES : OPTION_NO;
	auto thermal = bodyJson["Options"]["therm.corr"].get<bool>() ? OPTION_YES : OPTION_NO;
	auto aerojump = bodyJson["Options"]["aerojump"].get<bool>() ? OPTION_YES : OPTION_NO;

	return Options{(uint8_t)koriolis, (uint8_t)rangecard, (uint8_t)thermal, (uint8_t)aerojump};
}

Inputs s2::datapreparator::parseForInputs(const nlohmann::json& bodyJson) const {
	
	/* Parse for input data:
	"Inputs": {"dist.": 1000,"terrain_angle": 0,"target_azimuth": -15,"latitude": 54,"targ.speed": 2.3} */

	auto dist = bodyJson["Inputs"]["dist."].get<uint16_t>();
	auto terrain_angle = bodyJson["Inputs"]["terrain_angle"].get<uint8_t>();
	auto target_azimuth = bodyJson["Inputs"]["target_azimuth"].get<int16_t>();
	auto latitude = bodyJson["Inputs"]["latitude"].get<double>();
	auto targSpeed = bodyJson["Inputs"]["targ.speed"].get<double>();

	return Inputs{dist, terrain_angle, targSpeed, target_azimuth, latitude, 0}; //No magnetic inclination
}

Meteo s2::datapreparator::parseForMeteoData(const nlohmann::json& bodyJson) {

	/* Parse for meteo data "Meteo": {"temp.":15,"press.":1000,"humid.":50} */

	auto temp = bodyJson["Meteo"]["temp."].get<int8_t>();
	auto press = bodyJson["Meteo"]["press."].get<uint16_t>();
	auto humid = bodyJson["Meteo"]["humid."].get<uint8_t>();
	auto windType = bodyJson["Meteo"]["wind"].dump() == "\"simple\"" ? SIMPLE_CASE : COMPLEX_CASE;

	if(windType == SIMPLE_CASE) {

		auto windSpeed = bodyJson["Meteo"]["windage"][0]["speed"].get<double>();
		auto windDir = bodyJson["Meteo"]["windage"][0]["dir."].get<uint16_t>();
		auto terrainDir = bodyJson["Meteo"]["windage"][0]["incl."].get<int16_t>();

		return Meteo{temp, press, humid, windSpeed, windDir, terrainDir, (int8_t)windType, USELESS_COMPLEX_DATA};
	}
	else {

		memset(m_windArray, 0, sizeof(windDataArray));

		for(auto i = 0; i < WIND_GRANULARITY; ++i) {

			auto windDist = bodyJson["Meteo"]["windage"][i]["dist."].get<uint16_t>();
			auto windSpeed = bodyJson["Meteo"]["windage"][i]["speed"].get<double>();
			auto windDir = bodyJson["Meteo"]["windage"][i]["dir."].get<uint16_t>();
			auto windIncl = bodyJson["Meteo"]["windage"][i]["incl."].get<double>();

			m_windArray[i].currentDistance = windDist;
			m_windArray[i].windSpeed = windSpeed;
			m_windArray[i].windDir = windDir;
			m_windArray[i].terrainDir = windIncl;
		}

		return Meteo{temp, press, humid, USELESS_DATA, USELESS_DATA, USELESS_DATA, (int8_t)windType, &m_windArray};
	}
}

void s2::datapreparator::serializeResult(const Results& results, std::string& workBuffer) {
/*
{
    "Results": {
        "vert.": [104,5.45,54],
        "horiz.": [68,3.33,33], //Minus for wind from right side
        "deriv.": [10,0.1,1],
        "time": 0.86,
        "Mach": 1.05,
        "FGS": 1.41,
        "trg.move": 3.2,
        "cinetic": 4056,
        "trassonic": 752,
        "supersonic": 990,
        "subsonic": 1057,
        "rangecard": {
            "dist.": [50,75,100,125],
            "vert.": [0.51,0.68,1.2,1.33 ...],
            "horiz.": [0.33,0.42,0.8,1.22 ...],
            "deriv.": [0.1,0.1,0.1,0.12 ...],
            "time": [0.02,0.08,0.12,0.22 ...]
        }
    }
}
*/
	nlohmann::json responceJson;
	responceJson["Version"] = version;
	responceJson["Token"] = m_token;
	
	responceJson["Result"]["vert."] = {results.vertSm, results.vertAngleUnits, results.vertClicks};
	responceJson["Result"]["vert.abs"] = results.vertSmABS;
	responceJson["Result"]["horiz."] = {results.horizSm, results.horizAngleUnits, results.horizClicks};
	responceJson["Result"]["deriv."] = {results.derivSm, results.derivAngleUnits, results.derivClicks};
	responceJson["Result"]["time"] = results.flightTime;
	responceJson["Result"]["Mach"] = results.MachNumber;
	responceJson["Result"]["FGS"] = results.FGS;
	responceJson["Result"]["A0"] = results.A0;
	responceJson["Result"]["trg.move"] = results.targetAdvance;
	responceJson["Result"]["cinetic"] = results.cineticEnergy;	
	responceJson["Result"]["transsonic"] = results.transsonicDist;
	responceJson["Result"]["supersonic"] = results.subsonicDist;
	responceJson["Result"]["subsonic"] = results.deepSubsonic;
    responceJson["Result"]["subsonic0.7M"] = results.deepSubsonic_0_7M;
	responceJson["Result"]["transsonic2.2M"] = results.deeptranssonic_2_2M;
	responceJson["Result"]["transsonic2.0M"] = results.deeptranssonic_2_0M;
	responceJson["Result"]["transsonic1.8M"] = results.deeptranssonic_1_8M;
	responceJson["Result"]["transsonic1.6M"] = results.deeptranssonic_1_6M;
	responceJson["Result"]["transsonic1.4M"] = results.deeptranssonic_1_4M;
	responceJson["Result"]["transsonic1.2M"] = results.deeptranssonic_1_2M;

	if(m_makeRangecard) {

		prepareRangecardData(results);

		responceJson["Result"]["rangecard"]["dist."] = m_distances;
		responceJson["Result"]["rangecard"]["vert."] = m_verticals;
		responceJson["Result"]["rangecard"]["horiz."] = m_horizontals;
		responceJson["Result"]["rangecard"]["deriv."] = m_derivations;
		responceJson["Result"]["rangecard"]["time"] = m_times;
	}

	workBuffer = responceJson.dump(4);
}

void s2::datapreparator::prepareRangecardData(const Results& results) {

	m_distances.clear();
	m_verticals.clear();
	m_horizontals.clear();
	m_derivations.clear();
	m_times.clear();

	m_distances.reserve(BALLISTIC_TABLE_SIZE + 1);
	m_verticals.reserve(BALLISTIC_TABLE_SIZE + 1);
	m_horizontals.reserve(BALLISTIC_TABLE_SIZE + 1);
	m_derivations.reserve(BALLISTIC_TABLE_SIZE + 1);
	m_times.reserve(BALLISTIC_TABLE_SIZE + 1);

	for(int i = 1; i < BALLISTIC_TABLE_SIZE + 1; i++) {

		m_distances.push_back(i * TABLE_STEP);
		m_verticals.push_back(results.table.Vert[i][m_unitsIsMrads]);
		m_horizontals.push_back(results.table.Horiz[i][m_unitsIsMrads]);
		m_derivations.push_back(results.table.Deriv[i][m_unitsIsMrads]);
		m_times.push_back(results.table.Time[i]);
	}
}

void s2::datapreparator::getToken(const nlohmann::json& bodyJson) {

	m_token = bodyJson["Token"].get<std::string>();
}

void s2::solveBallistics(const std::string& inputJson, std::string& workBuffer) {

	LOG_INFO(fastlog::LogEventType::System) << "Приняты входные данные: " << inputJson;

	workBuffer.reserve(BALLISTIX_WORKING_BUFFER_SIZE);

	try {
		
		auto bodyJson = nlohmann::json::parse(inputJson);

		s2::datapreparator dp;

		dp.getToken(bodyJson);

		auto bullet = dp.parseForBulletData(bodyJson);
		auto rifle = dp.parseForRifleData(bodyJson);
		auto scope = dp.parseForScopeData(bodyJson);
		auto meteo = dp.parseForMeteoData(bodyJson);
		auto options = dp.parseForOptions(bodyJson);
		auto inputs = dp.parseForInputs(bodyJson);

		Results results;
		trajectorySolver(&meteo, &bullet, &rifle, &scope, &inputs, &options, OUT &results);

		dp.serializeResult(results, workBuffer);

		LOG_INFO(fastlog::LogEventType::System) << "Результат вычислений: " << workBuffer;
		return;
	}
	catch(...) {

		workBuffer = "{}";
		LOG_INFO(fastlog::LogEventType::System) << "Данные не обработаны";
		return;
	}	
}