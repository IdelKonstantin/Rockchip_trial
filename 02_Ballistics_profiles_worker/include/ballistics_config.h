#ifndef _BALLISTIX_PROFILES_KEEPER_H_
#define _BALLISTIX_PROFILES_KEEPER_H_

#include <cstdint>
#include <map>
#include <limits>
#include <string>

#include "trajectory_solver_API.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_BULLETS_QUANTITY 10
#define MAX_RIFLES_QUANTITY 10
#define FLOAT_EPSILON std::numeric_limits<float>::epsilon()

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define BALLISTICS_DEFAULT_BULLET_INDEX			0
#define BALLISTICS_DEFAULT_BULLET_PREFIX		"Bullet_"
#define BALLISTICS_DEFAULT_BULLET_DF			G7
#define BALLISTICS_DEFAULT_BULLET_BC			0.200f
#define BALLISTICS_DEFAULT_BULLET_MV			800
#define BALLISTICS_DEFAULT_BULLET_LENGHT		30.0f
#define BALLISTICS_DEFAULT_BULLET_WEIGHT		150
#define BALLISTICS_DEFAULT_BULLET_DIAM			7.8
#define BALLISTICS_DEFAULT_BULLET_MV_TEMP		15
#define BALLISTICS_DEFAULT_BULLET_THERMAL		0.0f
#define BALLISTICS_DEFAULT_BULLET_CFACTOR		1.0f

#define BALLISTICS_DEFAULT_RIFLE_INDEX			0
#define BALLISTICS_DEFAULT_RIFLE_PREFIX			"Rifle_"
#define BALLISTICS_DEFAULT_RIFLE_ZERO_AT		100
#define BALLISTICS_DEFAULT_RIFLE_SCOPEHIGHT		5.0
#define BALLISTICS_DEFAULT_RIFLE_TWIST			255
#define BALLISTICS_DEFAULT_RIFLE_TWIST_DIR		RIGHT_TWIST
#define BALLISTICS_DEFAULT_RIFLE_ZEROING		HERE
#define BALLISTICS_DEFAULT_RIFLE_TEMP			15
#define BALLISTICS_DEFAULT_RIFLE_PRESS			1013
#define BALLISTICS_DEFAULT_RIFLE_VERT_DRIFT		0.0f
#define BALLISTICS_DEFAULT_RIFLE_VERT_DR_DIR	POI_UP
#define BALLISTICS_DEFAULT_RIFLE_HORIZ_DRIFT	0.0f
#define BALLISTICS_DEFAULT_RIFLE_HORIZ_DR_DIR	POI_LEFT
#define BALLISTICS_DEFAULT_RIFLE_SCOPE_UNITS	MRAD_UNITS
#define BALLISTICS_DEFAULT_RIFLE_VERT_CLICK		0.1f
#define BALLISTICS_DEFAULT_RIFLE_HORIZ_CLICK	0.1f

#define BALLISTICS_DEFAULT_LATITUDE				55
#define BALLISTICS_DEFAULT_DISTANCE				600
#define BALLISTICS_DEFAULT_ANGLE				0
#define BALLISTICS_DEFAULT_SPEED_MILLS			0.0f
#define BALLISTICS_DEFAULT_AZIMUTH				0
#define BALLISTICS_DEFAULT_WIND_ANGLE			90

#define BALLISTICS_DEFAULT_KORIOLIS_OPTION		false
#define BALLISTICS_DEFAULT_THERMAL_OPTION		false
#define BALLISTICS_DEFAULT_RANGECARD_OPTION		true
#define BALLISTICS_DEFAULT_AEROJUMP_OPTION		false

#define BALLISTICS_DEFAULT_MILDOT_SIZE			1.7f
#define BALLISTICS_DEFAULT_MILDOT_MILS			1.0f

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace ballistics {

	typedef struct bullet {

		std::string name;
		uint8_t DF;
		float BC;
		uint16_t MV;
		float length;
		uint16_t weight;
		float caliber;
		int8_t MV_temp;
		float thermSens;
		float CF_M0_9;
		float CF_M1_0;
		float CF_M1_1;

		bool operator!=(const bullet& other) const;
		bullet& operator=(const bullet& other);

	} bullet_t;

	typedef struct bullets {

		uint8_t indexOfSelected;
		bullet_t bullet[MAX_BULLETS_QUANTITY];

	} bullets_info_t;

	typedef struct rifle {

		std::string name;
		uint16_t zero_dist;
		float scope_hight;
		float twist;
		uint8_t twist_dir;
		uint8_t zeroing;
		int8_t zero_T;
		uint16_t zero_P;
		float vert_drift;
		uint8_t vert_drift_dir;
		float horiz_drift;
		uint8_t horiz_drift_dir;
		uint8_t scope_units;
		float vert_click;
		float horiz_click;

		bool operator!=(const rifle& other) const;
		rifle& operator=(const rifle& other);

	} rifle_t;

	typedef struct rifles {

		uint8_t indexOfSelected;
		rifle_t rifle[MAX_RIFLES_QUANTITY];

	} rifles_info_t;
};

namespace the_device {

	typedef struct settings {

		float latitude;
		float magIncl;

		bool operator!=(const settings& other) const;
		settings& operator=(const settings& other);

	} settings_t;
};

namespace BC {

	typedef struct inputs {

		bool koriolis{false};
		bool termoCorr{false};
		bool rangeCard{false};
		bool aeroJump{false};

		bool operator!=(const inputs& other) const;
		inputs& operator=(const inputs& other);

	} inputs_t;

	typedef struct target {

		uint16_t distance;
		uint8_t terrainAngle;
		float speedMILs;
		uint16_t azimuth;
		uint16_t windAngle;
		int8_t roll_angle;

		bool operator!=(const target& other) const;
		target& operator=(const target& other);

	} target_info_t;

	typedef struct mildot {

		float sizeMeters;
		float sizeMils;

		bool operator!=(const mildot& other) const;
		mildot& operator=(const mildot& other);

	} mildot_inputs_t;
};

#endif /* _BALLISTIX_PROFILES_KEEPER_H_ */