#ifndef _SOLVER_STRUCTS_AND_CONSTS_H_
#define _SOLVER_STRUCTS_AND_CONSTS_H_

#include <stdint.h>

const double STEP_f = 3.28084;
const double DegToRad = 0.0174533;
const double hPa2mmHg = 0.75;
const double mmToInch = 0.03937;
const double MOAToMRAD = 0.2909;
const double grain2Pounds = 0.00014286;

const double Pi = 3.14159;
const double Inch = 25.4;
const double InchInSm = 2.54;
const double EarthRotation = 0.00007292;
const double MOA_ = 2.9089;
const double MRAD_ = 10.0;
const double _MPHtoMPS = 2.237;

/* Это умноженные на 100 дистанции для калибровки */
const uint16_t deeptranssonic22Mach_ = 220;
const uint16_t deeptranssonic20Mach_ = 200;
const uint16_t deeptranssonic18Mach_ = 180;
const uint16_t deeptranssonic16Mach_ = 160;
const uint16_t deeptranssonic14Mach_ = 140;
const uint16_t deeptranssonic12Mach_ = 120;

const uint16_t transsonicMach_ = 110;
const uint16_t subsonicMach_ = 100;
const uint16_t deepSubsonicMach_ = 90;
const uint16_t deepSubsonicMach07Mach_ = 70;

typedef struct {

	uint16_t Dist;		/* Dist, (meters) */
	double Dst_f;		/* Dist, (feets) */
	double Yabs;			/* Y, (sm) absolute */
	double Yrz;			/* Y, (sm) relative to zero point (ZP) */
	double Yrt;			/* Y', (sm) relative to ZP and terrain angle */
	double W;			/* windage (sm) */
	double Time;			/* (sec) */
	double Deriv;		/* (sm) */
	double Yrt_moa;		/* in MOA */
	double Yrt_mrad;		/* in MRAD */
	double W_moa;		/* in MOA */
	double W_mrad;		/* in MRAD */
	double Deriv_moa;	/* in MOA */
	double Deriv_mrad;	/* in MRAD */
	double Koriol_h;		/* Horizontal Koriolis */
	double MachNumber;	/* Mach number at distance */
	double windSpeed; 

} solverUnit;

struct solveCompactData {

	uint16_t dist;
	double Yabs; 
	double W; 
	double Time;
	double Deriv;
	double Mach;
	double Wz;
};

struct windPortion {

	double Wx;
	double Wy;
	double Wz;
};

struct dragAndBCInfo {

	double CD;
	double C3;
	double BCzero;
};

struct calibrationDistances {

	uint16_t DistTrans22M;
	uint16_t DistTrans20M;
	uint16_t DistTrans18M;
	uint16_t DistTrans16M;
	uint16_t DistTrans14M;
	uint16_t DistTrans12M;

	uint16_t DistTrans;			//1.1M
	uint16_t DistSubsonic; 		//1.0M
	uint16_t DistDeepSubsonic;	//0.9M
	uint16_t DistDeepSubsonic07M;	//0.7M
};

struct zeroingInfo {

	double DistFeet;
	double Yzero;
};

struct CalibCoefficients {

	double atTrassonic;
	double atSubsonic;
	double atDeepSubsonic;
};

struct VertCorrsAtCalibDists { /* vertical corrections at 1.1, 1.0 and 0.9 Mach */

	double verCorrAtTranssonic;
	double verCorrAtSubsonic;
	double verCorrAtDeepSubsonic;
};

struct terminalData {

	double VxDist;
	double MachDist;
	uint32_t cineticEnergy;
};

#endif /* _SOLVER_STRUCTS_AND_CONSTS_H_ */
