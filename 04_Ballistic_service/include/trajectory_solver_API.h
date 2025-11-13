#ifndef __TRAJECTORY_SOLVER_API_H__
#define __TRAJECTORY_SOLVER_API_H__

#include <stdint.h>

//#define PORTABLE_DEVICE		/* Should be defined for handhelds*/
//#define SLOW_DEVICE 			/* Should be defined for handhelds*/

#define DIST_RANGE 4000			/* Maximum distance in ballistic table, aliquot of TABLE_STEP (!) */
#define TABLE_STEP 25			/* Ballistic table step in meters 25, 50, 100 */
#define WIND_GRANULARITY		5	/* Quantity of wind measurement points */
#define CMD_GRANULARITY			31	/* Quantity of custom drag-function points (0.5 - 3.5 Mach) */
#define MBC_GRANULARITY			26  /* Quantity of custom multiBC points (0.5 - 3.0 Mach) */
#define USELESS_DATA			0
#define USELESS_COMPLEX_DATA	NULL

#define BALLISTIC_TABLE_SIZE DIST_RANGE/TABLE_STEP
#define OUT

// #if defined SLOW_DEVICE
#define MAX_NAME_SIZE 52U
// #else
// #define MAX_NAME_SIZE 255U
// #endif

enum angleUnits {

	MOA_UNITS = 1,
	MRAD_UNITS = 2,
};

enum dragModels {
	
	G1 = 1,
	G7 = 7,
	Gs = 99,	/* For spheres */
	CDM = 15,	/* For custom DF */
	MBCG1 = 9,	/* For multiBC G1 */
	MBCG7 = 21, /* For multiBC G7 */
};

enum twistDirection {

	RIGHT_TWIST = 0,
	LEFT_TWIST = 1,
};

enum POIDrift {

	POI_LEFT = 0,
	POI_RIGHT = 1,
	POI_UP = 0,
	POI_DOWN = 1,
};

enum inputOptions {

	OPTION_NO = 0,	
	OPTION_YES = 1,
};

enum zeroAtmosphere {

	HERE = 0,
	NOT_HERE = 1,
};

enum TSerrorCodes {

	TS_SUCCESS = 0,
	TS_FAIL = 1,
};

enum reticlePatterns {

    MIL_DOT = 1,
};

enum WindType {

    SIMPLE_CASE = 0, 	/* Equal wind on whole distance */
    COMPLEX_CASE = 1, 	/* Random wind */
};

/****************************************************************
*
*	0 - 360 wind direction angle. Clockwise
*   0 - zero ground angle
*   - - clockwise ground angle
*   + - counterclockwise ground angle  
*
*   see below (ComplexWind)
* 
****************************************************************/

typedef struct ComplexWind {

	uint16_t currentDistance;
	double windSpeed;
	uint16_t windDir;
	double terrainDir;

} windDataArray[WIND_GRANULARITY];

struct Meteo {

	int8_t T;
	uint16_t P;
	uint8_t H;

	/* Simple wind case */
	double windSpeed;
	uint16_t windDir;
	int16_t terrainDir;

	/* Flag for wind type definition */
	int8_t WindType;

	/* Random wind case - here is array of complex 
	wind data (dist, velocity, direction) with 
	dimension equal to WIND_GRANULARITY */
	windDataArray* windData;
};

/* Usage of windData field:

	windDataArray windArray = {{500, 4.0, 90}, {600, 5.5, 180}, ...}

	struct Meteo meteo;
	meteo.windData = &windArray;

	(*meteo.windData)[1].currentDistance
	(*meteo.windData)[1].windSpeed
	(*meteo.windData)[1].windDir

	or put NULL with SIMPLE_CASE

 */

typedef struct CDMportion {

	double MachNumber;
	double CD;
} CDMDataArray[CMD_GRANULARITY];


typedef struct MBCportion {

	double MachNumber;
	double BC;
} MBCDataArray[MBC_GRANULARITY];


struct Bullet {

	char bulletName[MAX_NAME_SIZE];
	uint8_t dragFunction;
	/* Data for classic G-functions */
	double BC; 
	double DSF_0_9;
	double DSF_1_0;
	double DSF_1_1;
	uint16_t V0; 
	double length;
	uint16_t mass;
	double caliber;
	int8_t V0temp;
	double thermalSens;
	
	/* Data for CDMs */
	CDMDataArray* cdmData;
	MBCDataArray* mbcData;
};

/************************************************************** 
**** Usage of cdmData field:

	CDMDataArray CDMArray = {{0.5, 0.16}, {0.6, 0.166}, ...}

	struct Bullet bullet;
	bullet.cdmData = &CDMArray;

	(*bullet.cdmData)[1].MachNumber
	(*bullet.cdmData)[1].CD

	or put NULL if use G's

**** Usage of mbcData field:

	MBCDataArray MBCArray = {{0.5, 0.212}, {0.6, 0.0215}, ...}

	struct Bullet bullet;
	bullet.mbcData = &MCDArray;

	(*bullet.mcdData)[1].MachNumber
	(*bullet.mcdData)[1].BC

	or put NULL if use G's
**************************************************************/

struct Rifle {

	char rifleName[MAX_NAME_SIZE];
	uint16_t zeroDistance;
	double scopeHight;
	double twist;
	uint8_t twistDir;
	uint8_t zeroAtm;
	int8_t zeroTemp;
	uint16_t zeroPress;
	double vertDrift;
	uint8_t vertDrDir;
	double horizDrift;
	uint8_t horizDrDir;
	int16_t rollAngle;
};

struct Scope {

	char scopeName[MAX_NAME_SIZE];
	uint8_t angleUnits;
	double clickVert;
	double clickHoriz;
	uint8_t reticlePattern;
};

struct Inputs {

	uint16_t shotDistance;
	uint8_t terrainAndle;
	double targetSpeedInMILs;
	int16_t targetAzimuth;
	double latitude;
	double magneticIncl;
};

struct Options {

	uint8_t Koriolis;
	uint8_t BallisticTable; 
	uint8_t ThermalCorrection;
	uint8_t AeroJump;
};

struct BallisticTable {

	struct Meteo meteo;
	struct Bullet bullet;
	struct Rifle rifle;
	double Vert[BALLISTIC_TABLE_SIZE + 1][2];
	double Horiz[BALLISTIC_TABLE_SIZE + 1][2];
	double Deriv[BALLISTIC_TABLE_SIZE + 1][2];
	double Time[BALLISTIC_TABLE_SIZE + 1];
} ;

struct Results {

	/* detailed output*/
	char bulletName[MAX_NAME_SIZE];
	char rifleName[MAX_NAME_SIZE];
	int32_t vertSm;
	int32_t vertSmABS;
	double vertAngleUnits;
	int32_t vertClicks;
	int32_t horizSm;
	double horizAngleUnits;
	int32_t horizClicks;
	int32_t derivSm;
	double derivAngleUnits;
	int32_t derivClicks;
	double targetAdvance;
	double flightTime;
	uint32_t cineticEnergy;
	double MachNumber;
	uint16_t A0;
	double FGS;	
	uint16_t deeptranssonic_2_2M;
	uint16_t deeptranssonic_2_0M;
	uint16_t deeptranssonic_1_8M;
	uint16_t deeptranssonic_1_6M;
	uint16_t deeptranssonic_1_4M;
	uint16_t deeptranssonic_1_2M;
	uint16_t transsonicDist;        //1.1
	uint16_t subsonicDist;          //1.0
	uint16_t deepSubsonic;          //0.9
	uint16_t deepSubsonic_0_7M;     //0.7

	struct BallisticTable table;
};

#endif /* __TRAJECTORY_SOLVER_API_H__ */
