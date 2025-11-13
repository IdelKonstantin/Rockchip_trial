#include "trajectory_solver.h"
#include "roll_angle.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static solverUnit solver[BALLISTIC_TABLE_SIZE + 2]{};
static struct calibrationDistances calibDists;
static struct windPortion windComps;
static struct dragAndBCInfo dragInfo;
static struct zeroingInfo zeroData;
static struct solveCompactData solveData;
static struct terminalData terminalInfo;
static struct rifleAngles anglesData;
static const double dummy{0};
static struct roolCorrectionData rifleRollData;

static inline double calculateFullSpeedValue(double Vx, double Vy, double Vz) {
	
	return sqrt((Vx * Vx) + (Vy * Vy) + (Vz * Vz));               
}

static inline double calculateMach(double V, double A0_f) {

	return V * A0_f;
}

static inline double calculateScopeFeetOffset(const struct Rifle* const rifle) {

	return -convertToFeets(rifle->scopeHight * 0.01);
}

static inline void setZeroingInputs(uint16_t dist, double Elevation, const struct Rifle* const rifle, struct zeroingInfo* zeroData) {

	if (dist == rifle->zeroDistance) {

		zeroData->DistFeet = convertToFeets(dist);
		zeroData->Yzero = Elevation;
	}
}

static inline void setSolverOutput(const solveCompactData* const solveData, solverUnit* solver, uint64_t index) {

	solver[index].Dist = solveData->dist;
	solver[index].Dst_f = convertToFeets(solveData->dist);
	solver[index].Yabs = solveData->Yabs;
	solver[index].W = solveData->W;
	solver[index].Time = solveData->Time;
	solver[index].Deriv = solveData->Deriv;
	solver[index].MachNumber = solveData->Mach;
	solver[index].windSpeed = solveData->Wz;
}

static inline double calculateThrowingAngle(double G_f, const struct Rifle* const rifle, const struct Bullet* const bullet, 
	const struct Meteo* const meteo, const struct zeroingInfo* const zeroData, const struct dragAndBCInfo* const dragInfo) {

	if(rifle->zeroAtm == HERE) {
		return throwAngleCalculation(zeroData->Yzero, zeroData->DistFeet);
	}
	else if (rifle->zeroAtm == NOT_HERE) {
		return ZeroingAngleforNumeric(G_f, rifle, bullet, meteo, dragInfo);
	}

	return dummy;
}

static inline void fillRifleRollData(const struct Rifle* const rifle) {

	double MOAatDistance = MOA_ * (rifle->zeroDistance / 100.0);

	rifleRollData.MOAcorrection = (rifle->scopeHight / MOAatDistance);
	rifleRollData.MRADcorrection = rifleRollData.MOAcorrection * MOAToMRAD;
}

static void calculateComplexRoll(const struct Rifle* const rifle, const struct Scope* const scope, struct Results* OUT results) {

	resetAngles(&anglesData);
	anglesData = {results->vertAngleUnits, results->horizAngleUnits + results->derivAngleUnits};
	correctViaRollAngle(&anglesData, rifle->rollAngle);
	
	results->vertAngleUnits = anglesData.vertCorr;
	results->horizAngleUnits = anglesData.horizCorr;
	results->derivAngleUnits = 0;

	resetAngles(&anglesData);
	anglesData = {(double)results->vertSm, (double)results->horizSm + (double)results->derivSm};
	correctViaRollAngle(&anglesData, rifle->rollAngle);
	
	results->vertSm = (int16_t)anglesData.vertCorr;
	results->horizSm = (int16_t)anglesData.horizCorr;
	results->derivSm = 0;

	results->vertClicks = (int16_t)(results->vertAngleUnits / scope->clickVert);
	results->horizClicks = (int16_t)(results->horizAngleUnits / scope->clickHoriz);
	results->derivClicks = 0;
}

static void calculateSimpleRollAtZeroing(const struct Rifle* const rifle, const struct Scope* const scope, const struct Inputs* const inputs, struct Results* OUT results) {

	const double angleCorrectionFactor = scope->angleUnits == MOA_UNITS ? MOA_ : MRAD_;

	double VertDriftAngular = getVertDriftAngular(rifle, scope);
	double HorizDriftAngular = getHorizDriftAngular(rifle, scope);

	double rawVertAngleCorrection = ((scope->angleUnits == MOA_UNITS) ? rifleRollData.MOAcorrection : rifleRollData.MRADcorrection) + VertDriftAngular;
	double rawHorizAngleCorrection = HorizDriftAngular;

	/* При повороте на -90 градусов (поворот против часовой стрелки), скручиваем поправки по вертикали и накручиваем их на 
	горизонтальном маховике как для ветра под 90 градусов, т.е со знаком "-"
	*/

	double vertAngleCorrection = -rawVertAngleCorrection;
	double horizAngleCorrection = (rifle->rollAngle == -90) ? -rawVertAngleCorrection : rawVertAngleCorrection;

	/* При повороте на -90 градусов (поворот против часовой стрелки), скручиваем поправки на смещение пристрелки по горизонтали и накручиваем их на 
	вертикальном маховике. С учетом правила знаков, получается, что положительная поправка по горизонтали должна накручиваться в сторону увеличения 
	вертикальной. И соответтвенно откурчиваться в сторону минуса на горизонтальном маховике
	*/

	vertAngleCorrection = vertAngleCorrection + HorizDriftAngular;
	horizAngleCorrection = horizAngleCorrection - rawHorizAngleCorrection;

	results->vertAngleUnits = vertAngleCorrection;
	results->horizAngleUnits = horizAngleCorrection;

	int16_t vertSmCorrection = vertAngleCorrection * angleCorrectionFactor * (inputs->shotDistance / 100.0);
	int16_t horizSmCorrection = horizAngleCorrection * angleCorrectionFactor * (inputs->shotDistance / 100.0);

	results->vertSm = vertSmCorrection;
	results->horizSm = horizSmCorrection;

	results->vertClicks = (int16_t)(results->vertAngleUnits / scope->clickVert);
	results->horizClicks = (int16_t)(results->horizAngleUnits / scope->clickHoriz);	
}

static void calculateSimpleRollAtFar(const struct Rifle* const rifle, const struct Scope* const scope, const struct Inputs* const inputs, struct Results* OUT results) {

	const double angleCorrectionFactor = scope->angleUnits == MOA_UNITS ? MOA_ : MRAD_;

	double VertDriftAngular = getVertDriftAngular(rifle, scope);
	double HorizDriftAngular = getHorizDriftAngular(rifle, scope);

	double rawVertAngleCorrection = ((scope->angleUnits == MOA_UNITS) ? rifleRollData.MOAcorrection : rifleRollData.MRADcorrection) + VertDriftAngular;
	double rawHorizAngleCorrection = HorizDriftAngular;

	/* При повороте на -90 градусов (поворот против часовой стрелки), скручиваем поправки по вертикали и накручиваем их на 
	горизонтальном маховике как для ветра под 90 градусов, т.е со знаком "-"
	*/

	double vertAngleCorrection = -rawVertAngleCorrection;
	double horizAngleCorrection = (rifle->rollAngle == -90) ? -rawVertAngleCorrection : rawVertAngleCorrection;

	/* При повороте на -90 градусов (поворот против часовой стрелки), скручиваем поправки на смещение пристрелки по горизонтали и накручиваем их на 
	вертикальном маховике. С учетом правила знаков, получается, что положительная поправка по горизонтали должна накручиваться в сторону увеличения 
	вертикальной. И соответтвенно откурчиваться в сторону минуса на горизонтальном маховике
	*/

	double tempVertAngleCorrections = results->vertAngleUnits;
	double tempHorizAngleCorrections = results->horizAngleUnits; 

	vertAngleCorrection = vertAngleCorrection + HorizDriftAngular;
	horizAngleCorrection = horizAngleCorrection - rawHorizAngleCorrection;

	results->vertAngleUnits = vertAngleCorrection + tempHorizAngleCorrections;

	if(rifle->rollAngle == 90) {
		results->horizAngleUnits = horizAngleCorrection + tempVertAngleCorrections;
	}
	else {
		results->horizAngleUnits = horizAngleCorrection - tempVertAngleCorrections;
	}

	int16_t vertSmCorrection = results->vertAngleUnits * angleCorrectionFactor * (inputs->shotDistance / 100.0);
	int16_t horizSmCorrection = results->horizAngleUnits * angleCorrectionFactor * (inputs->shotDistance / 100.0);

	results->vertSm = vertSmCorrection;
	results->horizSm = horizSmCorrection;

	results->vertClicks = (int16_t)(results->vertAngleUnits / scope->clickVert);
	results->horizClicks = (int16_t)(results->horizAngleUnits / scope->clickHoriz);
}


void fillResultStructWithSimpleSolution(const struct Bullet* const bullet, const struct Rifle* const rifle,
	const struct Scope* const scope, const struct Inputs* const inputs,
	const struct terminalData* const terminalInfo, const struct calibrationDistances* const calibDists,
	double SG, struct Results* OUT results) {

	double Yrel = solver[BALLISTIC_TABLE_SIZE + 1].Yrt;
	double Wdrift = solver[BALLISTIC_TABLE_SIZE + 1].W;
	double time = solver[BALLISTIC_TABLE_SIZE + 1].Time;
	double deriv = solver[BALLISTIC_TABLE_SIZE+1].Deriv;

	double VertDriftAngular = getVertDriftAngular(rifle, scope);
	double HorizDriftAngular = getHorizDriftAngular(rifle, scope);

	double CorrectionFactor = CorrFactorAndMOAorMRAD(scope->angleUnits);
	Yrel = YwithPOIdrift(Yrel, VertDriftAngular, inputs->shotDistance, CorrectionFactor);
	Wdrift = WdwithPOIdrift(Wdrift, HorizDriftAngular, inputs->shotDistance, CorrectionFactor);
	double targetAdvanceInMils = targetAdvance(inputs->targetSpeedInMILs, time);

	ResultStructFullfilment(inputs->shotDistance, CorrectionFactor, time, targetAdvanceInMils, 
	terminalInfo->cineticEnergy, terminalInfo->MachDist, calibDists, SG, Yrel, Wdrift, deriv, 
	scope->clickVert, scope->clickHoriz, bullet->bulletName, rifle->rifleName, OUT results);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>

void trajectorySolver (const struct Meteo* const meteo, const struct Bullet* const bullet, const struct Rifle* const rifle, 
	const struct Scope* const scope, const struct Inputs* const inputs, const struct Options* const options, struct Results* OUT results) {

	if(rifle->rollAngle != 0) {
		
		fillRifleRollData(rifle);	
	}

	calibDists = {
		DIST_RANGE,
		DIST_RANGE,
		DIST_RANGE,
		DIST_RANGE,
		DIST_RANGE,
		DIST_RANGE,
		DIST_RANGE,
		DIST_RANGE,
		DIST_RANGE,
		DIST_RANGE
	};

	double V0 = V0dueToSensivity(meteo, bullet, options);
	double KoriolisVert = VerticalCoriolis(V0, inputs, options);
	double CCF = ConditionCorrectionFactor(meteo);
	double A0_f = speedOfSoundFeetRatio(meteo);
	double SG = MillersFGS(V0, meteo, bullet, rifle);
	double YaeroJump = aeroJmpCorrector(SG, bullet, rifle, meteo, options);	
	double G_f = gravityAccelerFeets(inputs);

	initStartWindComponents(meteo, &windComps);

	double Time = 0;

	double Vx1 = convertToFeets(V0);
	double Vy1, Vz1, W2;
	Vy1 = Vz1 = W2 = 0;

	double V_1 = calculateFullSpeedValue(Vx1, Vy1, Vz1);
	double M = calculateMach(V_1, A0_f);
	double H2 = calculateScopeFeetOffset(rifle);

	/******************** main ballistic calculation (START) ********************/
	for (uint16_t i = 0; i <= DIST_RANGE; i++) {

		defineDragInfoForCDM(i, M, CCF, bullet, rifle, &dragInfo, &calibDists);
		windComponentsForComplexCase(meteo, &windComps, i);
		
		double C4 = (dragInfo.CD * dragInfo.C3 * V_1) / Vx1;
		double A1_ = C4 * (Vx1 - windComps.Wx);
		double A2_ = (C4 * (Vy1 - windComps.Wy)) - (G_f / Vx1);
		double A3_ = C4 * (Vz1 - windComps.Wz);

		double Vx2 = Vx1 + convertToFeets(A1_);
		double Vy2 = Vy1 + convertToFeets(A2_);
		double Vz2 = Vz1 + convertToFeets(A3_);
		double V_2 = calculateFullSpeedValue(Vx2, Vy2, Vz2);

		//std::cout << i << " CD:" << dragInfo.CD << " C3:" << dragInfo.C3 << " C4:" << C4 << " A1_:" << A1_ 
		//<< " A2_:" << A2_ << " A3_:" << A3_ << " Vx2:" << Vx2 << " Vy2:" << Vy2 << " Vz2:" << Vz2 << " V_2:" << V_2 <<  std::endl; 



		M = calculateMach(V_2, A0_f);

		double C5 = (dragInfo.CD * dragInfo.C3 * V_2) / Vx2;
		double A4_ = C5 * (Vx2 - windComps.Wx);
		double A5_ = (C5 * (Vy2 - windComps.Wy)) - (G_f / Vx2);
		double A6_ = C5 * (Vz2 - windComps.Wz);

		double Vx3 = Vx1 + convertToFeets(0.5 * (A1_ + A4_));
		double Vy3 = Vy1 + convertToFeets(0.5 * (A2_ + A5_));
		double Vz3 = Vz1 + convertToFeets(0.5 * (A3_ + A6_));
		double V_3 = calculateFullSpeedValue(Vx3, Vy3, Vz3);
		M = calculateMach(V_3, A0_f);

		H2 = H2 + convertToFeets((Vy1 + Vy3) / (Vx1 + Vx3));	/* elevation in feets (absolute) */
		double Y = H2 / convertToFeets(0.01);					/* elevation in sm (absolute) */
		W2 = W2 + convertToFeets(((Vz1 + Vz3) / (Vx1 + Vx3)));	/* wind drift in feets */
		double W = W2 / convertToFeets(0.01);					/* wind drift in sm */
		Time = Time + (convertToFeets(2) / (Vx2 + Vx3));		/* flight time */

		setZeroingInputs(i, Y, rifle, &zeroData);

		if (i == inputs->shotDistance) {

			solveData = {i, Y, W, Time, DerivationCalculation(SG, Time, rifle->twistDir), M, convertFromFeets(windComps.Wz)};
			setSolverOutput(&solveData, solver, BALLISTIC_TABLE_SIZE + 1);

			double VxDist = convertFromFeets(V_1);
			terminalInfo = {VxDist, M, CineticEnergy(VxDist, bullet->mass)};

			//Added for barsuk
			results->vertSmABS = calculateAbsDrop(Y);
			results->A0 = SpeedOfSoundRaw(meteo);
		}

		if(options->BallisticTable == OPTION_YES) {

			if (i % TABLE_STEP == 0) {
				
				solveData = {i, Y, W, Time, DerivationCalculation(SG, Time, rifle->twistDir), M, convertFromFeets(windComps.Wz)};
				setSolverOutput(&solveData, solver, i/TABLE_STEP);
			}	
		}

		V_1 = V_3; Vx1 = Vx3; Vy1 = Vy3; Vz1 = Vz3;
		
	} /******************** main ballistic calculation (END) ********************/

	double throwAngle = calculateThrowingAngle(G_f, rifle, bullet, meteo, &zeroData, &dragInfo);
	addSomeSolutionDataToSolverStruct(KoriolisVert, YaeroJump, throwAngle, inputs, options, bullet, solver, BALLISTIC_TABLE_SIZE + 1, meteo);
	fillResultStructWithSimpleSolution(bullet, rifle, scope, inputs, &terminalInfo, &calibDists, SG, OUT results);

	if(options->BallisticTable == OPTION_YES) {
		
		for (uint16_t i = 0; i <= BALLISTIC_TABLE_SIZE; i++) {
			
			addSomeSolutionDataToSolverStruct(KoriolisVert, YaeroJump, throwAngle, inputs, options, bullet, solver, i, meteo);
		}
		fullfillBallisticTable(meteo, bullet, rifle, solver, scope, results);
	}

	if(rifle->rollAngle != 0) {
		if(rifle->rollAngle == -90 || rifle->rollAngle == 90) { 

			if(inputs->shotDistance == rifle->zeroDistance) {
				calculateSimpleRollAtZeroing(rifle, scope, inputs, results);
			}
			else {
				calculateSimpleRollAtFar(rifle, scope, inputs, results);
			}
		}
		else {
			calculateComplexRoll(rifle, scope, results);
		}
	}
}