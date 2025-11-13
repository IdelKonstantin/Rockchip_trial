#ifndef __TRAJECTORY_SOLVER_ROUTINES_H__
#define __TRAJECTORY_SOLVER_ROUTINES_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "trajectory_solver_API.h"
#include "solver_structs_and_consts.h"

/* INFO - should be in actual state */
#define LIB_VERSION "2.0.0.6"
#define LIB_TYPE "x86_64bit"

const char* libInfo (void);
const char* devInfo (void);

inline double OverallAirDensity(double Pa, double Ph, int8_t T);
double SpeedOfSound (double Pa, double Ph, int8_t T);

double speedOfSoundFeetRatio(const struct Meteo* const meteo);
double VapourPressure (const struct Meteo* const meteo);
double ConditionCorrectionFactor (const struct Meteo* const meteo);

double ZeroingAngleforNumeric(double G_f, const struct Rifle* const rifle, const struct Bullet* const bullet, 
	const struct Meteo* const meteo, const struct dragAndBCInfo* const dragInfo);

double throwAngleCalculation (double Y, double Dst_f);
double DragCoefficient (uint8_t DragFunction, double M);
double DragCoefficientForCDM (const struct Bullet* bullet, double Mach);
double linearInterpolationDSF (const struct Bullet* bullet, double Mach);
double KleroFormula (double Lat);
double V0dueToSensivity(const struct Meteo* const meteo, const struct Bullet* const bullet, const struct Options* const options);
double VerticalCoriolis(double V0, const struct Inputs* const inputs, const struct Options* const options);
void getCalibrationCoeffs(const struct VertCorrsAtCalibDists* calcCorrs, 
const struct VertCorrsAtCalibDists* realCorrs, struct OUT CalibCoefficients* callibCoeff);

double CorrFactorAndMOAorMRAD (uint8_t ScopeIn);
double YwithPOIdrift (double Y, double VertDriftAngular, uint16_t Dist, double CorrectionFactor);
double WdwithPOIdrift (double Wd, double HorizDriftAngular, uint16_t Dist, double CorrectionFactor);

double MillersFGS(double V0, const struct Meteo* const meteo, const struct Bullet* const bullet, 
const struct Rifle* const rifle);
double DerivationCalculation (double SG, double Time, uint8_t TwistDir);
uint16_t CineticEnergy (double Vx, double Weight);

void ResultStructFullfilment (uint16_t Dist, double CorrectionFactor, double Time, double targetAdvanceInMils, 
	uint16_t cineticEnergy, double Mach, const struct calibrationDistances* const calibDists, double SG, double Y, double Wd, 
	double Deriv, double ClickVert, double ClickHoriz, const char* bulletName, const char* rifleName, struct Results* OUT results);

double koriolisHoriz (uint16_t Dist, double Time, double Latitude);
double koriolisVert (double V0, uint16_t Azimuth, double MagIncl, double Latitude, double G);
double convertSmToMOA (double Yrt, uint16_t Dist);
double convertMOAToMRAD (double Yrt_MOA_);
double absoluteDropToZeroing (double Yabs, double Dst_f, double throwAngle);
double relativeDropWithTerrainAngle (double Yrz, double Yabs, double Alpha);
double targetAdvance(double Vtarget, double flightTime);
uint8_t startIndexForCDM(double Mach);
double gravityAccelerFeets(const struct Inputs* const inputs);
double getVertDriftAngular(const struct Rifle* const rifle, const struct Scope* const scope);
double getHorizDriftAngular(const struct Rifle* const rifle, const struct Scope* const scope);
double calculateFakeG7BC(const uint16_t bulletMass, double caliberInch, double i_7);
bool machAtTranssonic(const double MachNum);
bool machAtSubsonic(const double MachNum);
bool machAtDeepSubsonic(const double MachNum);
void fullfillBallisticTable(const struct Meteo* meteo, const struct Bullet* bullet, 
    const struct Rifle* rifle, const solverUnit solver[], const struct Scope* const scope, struct Results* OUT results);
void getWindComponents(const struct Meteo* meteo, const uint16_t dist, struct OUT windPortion* wind);
double aeroJmpCorrector(double MillersFGS, const struct Bullet* bullet, const struct Rifle* const rifle, 
	const struct Meteo* const meteo, const struct Options* const options);
void initStartWindComponents(const struct Meteo* const meteo, struct windPortion* windComps);
void windComponentsForComplexCase(const struct Meteo* const meteo, struct windPortion* windComps, uint16_t currentDist);
void addSomeSolutionDataToSolverStruct(double KoriolisVert, double YaeroJump, double throwAngle, const struct Inputs* const inputs, 
    const struct Options* const options, const struct Bullet* const bullet, solverUnit* solver, uint64_t index, const struct Meteo* const meteo);

void defineDragInfoForCDM(uint16_t dist, double Mach, double CCF, const struct Bullet* const bullet, 
	const struct Rifle* const rifle, struct dragAndBCInfo* dragInfo, struct calibrationDistances* calibDists);
double calculateC3(double CCF, double BC);
double BCforMBCCase (const struct Bullet* bullet, double Mach);
uint8_t startIndexForMBC(double Mach);
uint16_t SpeedOfSoundRaw(const struct Meteo* const meteo);
int32_t calculateAbsDrop(double Yabs);
double convertToFeets(double value);
double convertFromFeets(double value);

#endif /* __TRAJECTORY_SOLVER_ROUTINES_H__ */
