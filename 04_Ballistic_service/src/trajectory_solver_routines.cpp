#include "trajectory_solver_routines.h"

#define WIND_GRANULARITY_FOR_COMPLEX 5

const char* libInfo (void) {return LIB_VERSION;}
const char* devInfo (void) {return LIB_TYPE;}

void getCalibrationCoeffs(const struct VertCorrsAtCalibDists* calcCorrs, 
const struct VertCorrsAtCalibDists* realCorrs, struct OUT CalibCoefficients* callibCoeff) {

	callibCoeff -> atTrassonic = realCorrs -> verCorrAtTranssonic / 
	calcCorrs -> verCorrAtTranssonic;
	
	callibCoeff -> atSubsonic = realCorrs -> verCorrAtSubsonic / 
	calcCorrs -> verCorrAtSubsonic;
	
	callibCoeff -> atDeepSubsonic = realCorrs -> verCorrAtDeepSubsonic / 
	calcCorrs -> verCorrAtDeepSubsonic;
}

inline double OverallAirDensity(double Pa, double Ph, int8_t T) {

	double RHOa = (Pa-Ph)/(287.058*(T+273.15)); //dencity of drain air
	double RHOh = Ph/(461.495*(T+273.15)); //dencity of vapour
	return RHOa + RHOh; //overall dencity
}

double SpeedOfSound (double Pa, double Ph, int8_t T) {
	
	double RHOha = OverallAirDensity(Pa, Ph, T);
	return (sqrt(1.4*((Pa - Ph)/RHOha)));
}

uint16_t SpeedOfSoundRaw(const struct Meteo* const meteo) {

	double Ph = VapourPressure (meteo);
	double RHOha = OverallAirDensity(meteo->P * 100, Ph, meteo->T);
	return SpeedOfSound (meteo->P * 100, RHOha, meteo->T);
}

double speedOfSoundFeetRatio(const struct Meteo* const meteo) {

	double A0 = SpeedOfSoundRaw(meteo);
	return 1.0/(A0*STEP_f);	
}

double VapourPressure (const struct Meteo* const meteo) {

	return (meteo->H*6.1078*pow(10.0,((7.5*(meteo->T+273.15)-2048.625)/((meteo->T+273.15)-35.85))));
}

double ConditionCorrectionFactor (const struct Meteo* const meteo) {
	
	const double normalConditionDensity = 1.20288; //1.20889; 

	double Pa = meteo->P * 100;
	double Ph = VapourPressure (meteo);
	double RHOha = OverallAirDensity(Pa, Ph, meteo->T);
	return RHOha/normalConditionDensity;
}

double ZeroingAngleforNumeric(double G_f, const struct Rifle* const rifle, 
	const struct Bullet* const bullet, const struct Meteo* const meteo, const struct dragAndBCInfo* const dragInfo) {

	double H2, Y, M, CD;
	double Dst_f, C3, C4, C5, A1_, A2_, A3_, A4_, A5_, A6_;
	double Vx1, Vy1, Vz1;
	double Vx2, Vy2, Vz2, Vx3, Vy3, Vz3;
	double V_1, V_2, V_3;

	struct Meteo ZeroMeteo {
		
		.T = rifle->zeroTemp,
		.P = rifle->zeroPress,
		.H = 50
	};

	double Pa = rifle->zeroPress * 100;
	double CCF = ConditionCorrectionFactor(&ZeroMeteo);
	double Ph = VapourPressure (&ZeroMeteo);
	double A0 = SpeedOfSound (Pa, Ph, rifle->zeroTemp);

	struct Options ZeroOptions {

		OPTION_NO,
		OPTION_NO, 
		OPTION_YES,
		OPTION_NO,
	};

	///refactor (переписать под аналитическое решение)

	double V0_ = V0dueToSensivity(&ZeroMeteo, bullet, &ZeroOptions);

	Vx1 = V0_*STEP_f;
	Vy1 = 0;
	Vz1 = 0;
	H2 = -((rifle->scopeHight*0.01)*STEP_f);
	const double A0_f = 1/(A0*STEP_f);
	C3 = (-0.0002048757*CCF)/dragInfo->BCzero;
	V_1 = sqrt((Vx1*Vx1)+(Vy1*Vy1)+(Vz1*Vz1));
	M = V_1*A0_f;

	uint8_t DragFunction = bullet->dragFunction;

	if(DragFunction == CDM) {

		DragFunction = G7;
	}

	for (uint16_t i = 0; i <= rifle->zeroDistance; ++i) {

		Dst_f = i*STEP_f;
		CD = DragCoefficient (DragFunction, M);
		C4 = (CD*C3*V_1)/Vx1;
		A1_ = C4*(Vx1);
		A2_ = (C4*Vy1)-(G_f/Vx1);
		A3_ = C4*(Vz1);

		Vx2 = Vx1 + A1_*STEP_f;
		Vy2 = Vy1 + A2_*STEP_f;
		Vz2 = Vz1 + A3_*STEP_f;
		V_2 = sqrt((Vx2*Vx2)+(Vy2*Vy2)+(Vz2*Vz2));
		M = V_2*A0_f;

		C5 = (CD*C3*V_2)/Vx2;
		A4_ = C5*(Vx2);
		A5_ = (C5*Vy2)-(G_f/Vx2);
		A6_ = C5*(Vz2);    

		Vx3 = Vx1 + 0.5*(A1_+A4_)*STEP_f;
		Vy3 = Vy1 + 0.5*(A2_+A5_)*STEP_f;
		Vz3 = Vz1 + 0.5*(A3_+A6_)*STEP_f;
		V_3 = sqrt((Vx3*Vx3)+(Vy3*Vy3)+(Vz3*Vz3));
		M = V_3*A0_f;

		H2 = H2+((Vy1+Vy3)/(Vx1+Vx3))*STEP_f;//elevation in feets (absolete)
		Y = H2/(0.01*STEP_f);//elevation in sm (absolete)

		V_1 = V_3;
		Vx1 = Vx3;
		Vy1 = Vy3;
		Vz1 = Vz3;
	}

	return throwAngleCalculation(Y, Dst_f);
}

double throwAngleCalculation (double Y, double Dst_f) {
	return atan(Y / Dst_f);
}

double linearInterpolationDSF (const struct Bullet* bullet, double Mach) {

	double DSFbeg;
	double DSFend;
	double MachBeg;
	double MachEnd;

	if(Mach > 1.1) {

		DSFbeg = 1.0f;
		DSFend = bullet-> DSF_1_1;
		MachBeg = 4.0f;
		MachEnd = 1.1f;
	}

	else if(Mach <= 1.1 && Mach > 1.0) {

		DSFbeg = bullet-> DSF_1_1;
		DSFend = bullet-> DSF_1_0;
		MachBeg = 1.1f;
		MachEnd = 1.0f;
	}

	else if(Mach <= 1.0 && Mach > 0.9) {

		DSFbeg = bullet-> DSF_1_0;
		DSFend = bullet-> DSF_0_9;
		MachBeg = 1.0f;
		MachEnd = 0.9f;
	}

	else if(Mach <= 0.9) {

		DSFbeg = bullet-> DSF_0_9;
		DSFend = DSFbeg;
		MachBeg = 0.9f;
		MachEnd = 0.7f;
	}

    //X = f(X1)+( f(X2) - f(X1) )*(X - X1)/(X2 - X1)    
    return DSFbeg + (DSFend - DSFbeg)*(Mach - MachBeg)/(MachEnd - MachBeg);
}

double KleroFormula (double Lat) {

	return 9.78034+0.05164*(sin(Lat)*sin(Lat));  
}

double V0dueToSensivity(const struct Meteo* const meteo, const struct Bullet* const bullet, const struct Options* const options) {

	if(options->ThermalCorrection == OPTION_YES) {
		return bullet->V0 + (bullet->V0 * bullet->thermalSens * 0.00067 * (meteo->T-bullet->V0temp));
	}

	return bullet->V0;
}

double VerticalCoriolis(double V0, const struct Inputs* const inputs, const struct Options* const options) { 

	if(options->Koriolis == OPTION_YES) {	

		return koriolisVert (V0, inputs->targetAzimuth, inputs->magneticIncl, inputs->latitude, KleroFormula(inputs->latitude));
	}

	return 1.0;
}


double CorrFactorAndMOAorMRAD (uint8_t ScopeIn) {
	
	if (ScopeIn == MOA_UNITS) {
		return 2.9089;
	}	
	
	return 10.0; /* else -> Scope == MRAD */
}

double YwithPOIdrift (double Y, double VertDriftAngular, uint16_t Dist, double CorrectionFactor) {

	return (Y + (VertDriftAngular*((Dist/100.0)*CorrectionFactor))); 
}

double WdwithPOIdrift (double Wd, double HorizDriftAngular, uint16_t Dist, double CorrectionFactor) {

	return (Wd + (HorizDriftAngular*((Dist/100.0)*CorrectionFactor))); 
}

double MillersFGS (double V0, const struct Meteo* const meteo, const struct Bullet* const bullet, 
const struct Rifle* const rifle) {

	double V0_f, Bul_l_in_cal, Bul_c_in, Twist_in_cal, SG0, fv, ftp, Tfahr, Press_inHg;
	Twist_in_cal = rifle->twist / bullet->caliber;
	Bul_c_in = bullet->caliber / Inch;
	Bul_l_in_cal = bullet->length / bullet->caliber;
	SG0 = (30.0 * bullet->mass) / (pow(Twist_in_cal,2) * pow(Bul_c_in,3) * Bul_l_in_cal * (1.0 + pow(Bul_l_in_cal,2)));
	V0_f = V0 * STEP_f;
	fv = pow((V0_f / 2800.0),(0.33333));
	Tfahr = (meteo->T * 1.8) + 32.0;
	Press_inHg = meteo->P * hPa2mmHg / Inch;
	ftp = ((Tfahr + 460.0) / (519.0))*(29.92 / Press_inHg);
	
	return SG0 * fv * ftp;
}

double DerivationCalculation (double SG, double Time, uint8_t TwistDir) {

	double derivation = InchInSm * 1.25 * (SG+1.2) * pow(Time, 1.83);   
	(TwistDir==LEFT_TWIST) ? derivation *= -1 : 0;
	
	return derivation;
}

uint16_t CineticEnergy (double Vx, double Weight) {

	return (uint16_t)(0.5*Vx*Vx*0.0000648*Weight);
}

void ResultStructFullfilment (uint16_t Dist, double CorrectionFactor, double Time, double targetAdvanceInMils, 
	uint16_t cineticEnergy, double Mach, const struct calibrationDistances* const calibDists, double SG, double Y, double Wd, 
	double Deriv, double ClickVert, double ClickHoriz, const char* bulletName, const char* rifleName, struct Results* OUT results) {

	/* Name of bullet and rifle */
#ifndef SLOW_DEVICE	
	strcpy(results->bulletName, bulletName);
	strcpy(results->rifleName, rifleName);
#endif

	double sm2angle = ((Dist/100.0)*CorrectionFactor);

	results->vertSm = round(Y);
	results->vertAngleUnits = Y/sm2angle;
	results->vertClicks = round((Y/sm2angle)/ClickVert);

	/* Horizontal corrections */
	results->horizSm = round(Wd);
	results->horizAngleUnits = Wd/sm2angle;
	results->horizClicks = round((Wd/sm2angle)/ClickHoriz);

	/* Derivation corrections */
	results->derivSm = round(Deriv);
	results->derivAngleUnits = Deriv/sm2angle;
	results->derivClicks = round((Deriv/sm2angle)/ClickHoriz);

	/* Complex shot solution */ 
	results->targetAdvance = targetAdvanceInMils;
	results->flightTime = Time;
	results->cineticEnergy = cineticEnergy;
	results->MachNumber = Mach;

	results->deeptranssonic_2_2M = calibDists->DistTrans22M;
	results->deeptranssonic_2_0M = calibDists->DistTrans20M;
	results->deeptranssonic_1_8M = calibDists->DistTrans18M;
	results->deeptranssonic_1_6M = calibDists->DistTrans16M;
	results->deeptranssonic_1_4M = calibDists->DistTrans14M;
	results->deeptranssonic_1_2M = calibDists->DistTrans12M;

	results->transsonicDist = calibDists->DistTrans;
	results->subsonicDist = calibDists->DistSubsonic;
	results->deepSubsonic = calibDists->DistDeepSubsonic;
    results->deepSubsonic_0_7M = calibDists->DistDeepSubsonic07M;
	results->FGS = SG;
}

double koriolisHoriz (uint16_t Dist, double Time, double Latitude) {

	double Vavg = Dist/Time;
	return ((EarthRotation*(pow(Dist,2))*sin(DegToRad*Latitude))/(Vavg))*100;
}

double koriolisVert (double V0, uint16_t Azimuth, double MagIncl, double Latitude, double G) {

	return (1.0-((2*EarthRotation*V0*cos(DegToRad*Latitude)*sin(DegToRad*(Azimuth+MagIncl)))/G));
}

double convertSmToMOA (double Yrt, uint16_t Dist) {

	return Yrt/((Dist/100.0)*MOA_);
}

double convertMOAToMRAD (double Yrt_MOA_) {

	return Yrt_MOA_*MOAToMRAD;
}

double absoluteDropToZeroing (double Yabs, double Dst_f, double throwAngle) {

	double cosThrowAngle = cos(throwAngle);
	return fabs((Yabs * cosThrowAngle - Dst_f * sin(throwAngle)) / cosThrowAngle);
}

double relativeDropWithTerrainAngle (double Yrz, double Yabs, double Alpha) {

	return (Yrz - fabs(Yabs * (1 - cos((Pi * Alpha) / 180.0))));
}

double targetAdvance(double Vtarget, double flightTime) {

	return Vtarget*flightTime;
}

/* Look up table for CDM-data */
uint8_t startIndexForCDM(double Mach) {
    
#define MACH_0_5 0
#define MACH_3_5 30    
    
    if(Mach >= 3.5) {
        
        return MACH_3_5;
    }
    
    if(Mach < 0.5) {
        
        return MACH_0_5;
    }
    
    return (Mach*10) - 5;
}

double DragCoefficientForCDM (const struct Bullet* bullet, double Mach) {

	uint8_t startIndex = startIndexForCDM(Mach);
	uint8_t endIndex = startIndex + 1;

	double startMach = (*(*(bullet)).cdmData)[startIndex].MachNumber;
	double endMach = (*(*(bullet)).cdmData)[endIndex].MachNumber;

	double startCD = (*(*(bullet)).cdmData)[startIndex].CD;
	double endCD = (*(*(bullet)).cdmData)[endIndex].CD;

	// //X = f(X1)+( f(X2) - f(X1) )*(X - X1)/(X2 - X1) 
	double Cd = startCD + (endCD - startCD)*(Mach - startMach)/(endMach - startMach);
	return Cd;
}

double gravityAccelerFeets(const struct Inputs* const inputs) {

	return KleroFormula(inputs->latitude)*STEP_f;
}

double getVertDriftAngular(const struct Rifle* const rifle, const struct Scope* const scope) {

	double sm2AngleZero = (scope->angleUnits == MOA_UNITS) ? (rifle->zeroDistance / 100.0) * 2.9089 : rifle->zeroDistance / 10.0;
	double VertDriftAngular = rifle->vertDrift / sm2AngleZero;
	(rifle->vertDrDir == POI_UP) ? VertDriftAngular *= -1 : 0;
	return VertDriftAngular;
}

double getHorizDriftAngular(const struct Rifle* const rifle, const struct Scope* const scope) {

	double sm2AngleZero = (scope->angleUnits == MOA_UNITS) ? (rifle->zeroDistance / 100.0) * 2.9089 : rifle->zeroDistance / 10.0;
	double HorizDriftAngular = rifle->horizDrift / sm2AngleZero;
	(rifle->horizDrDir == POI_LEFT) ? HorizDriftAngular *= -1 : 0;
	return HorizDriftAngular;
}

double calculateFakeG7BC(const uint16_t bulletMass, double caliberInch, double i_7) {

	return bulletMass*grain2Pounds/(caliberInch*caliberInch*i_7);
}

bool machAtTranssonic(const double MachNum) {

    uint16_t MachNum_ = MachNum*100;
    return MachNum_ == transsonicMach_;
}

bool machAtSubsonic(const double MachNum) {

    uint16_t MachNum_ = MachNum*100;
    return MachNum_ == subsonicMach_;
}

bool machAtDeepSubsonic(const double MachNum) {

    uint16_t MachNum_ = MachNum*100;
    return MachNum_ == deepSubsonicMach_;
}

bool machAtDeepSubsonic07(const double MachNum) {

    uint16_t MachNum_ = MachNum*100;
    return MachNum_ == deepSubsonicMach07Mach_;
}

bool machAtDeepTranssonic12(const double MachNum) {

    uint16_t MachNum_ = MachNum*100;
    return MachNum_ == deeptranssonic12Mach_;
}

bool machAtDeepTranssonic14(const double MachNum) {

    uint16_t MachNum_ = MachNum*100;
    return MachNum_ == deeptranssonic14Mach_;
}

bool machAtDeepTranssonic16(const double MachNum) {

    uint16_t MachNum_ = MachNum*100;
    return MachNum_ == deeptranssonic16Mach_;
}

bool machAtDeepTranssonic18(const double MachNum) {

    uint16_t MachNum_ = MachNum*100;
    return MachNum_ == deeptranssonic18Mach_;
}

bool machAtDeepTranssonic20(const double MachNum) {

    uint16_t MachNum_ = MachNum*100;
    return MachNum_ == deeptranssonic20Mach_;
}

bool machAtDeepTranssonic22(const double MachNum) {

    uint16_t MachNum_ = MachNum*100;
    return MachNum_ == deeptranssonic22Mach_;
}


void fullfillBallisticTable(const struct Meteo* meteo, const struct Bullet* bullet, const struct Rifle* rifle,
                            const solverUnit solver[], const struct Scope* const scope, struct Results* OUT results) {

#ifndef PORTABLE_DEVICE
		results->table.meteo = *meteo;
		results->table.bullet = *bullet;
		results->table.rifle = *rifle;
#endif	
        double VertDriftAngular = getVertDriftAngular(rifle, scope);
        double HorizDriftAngular = getHorizDriftAngular(rifle, scope);

		for(uint16_t i = 0; i < BALLISTIC_TABLE_SIZE + 1; ++i) {

            if(scope->angleUnits == MOA_UNITS) {

                results->table.Vert[i][0] = solver[i].Yrt_moa + VertDriftAngular;		/* Vertical MOA */
                results->table.Horiz[i][0] = solver[i].W_moa + HorizDriftAngular; 		/* Horizontal MOA */
                results->table.Deriv[i][0] = solver[i].Deriv_moa + HorizDriftAngular; 	/* Derivation MOA */
            }
            else {

                results->table.Vert[i][1] = solver[i].Yrt_mrad + VertDriftAngular;		/* Vertical MRAD */
                results->table.Horiz[i][1] = solver[i].W_mrad + HorizDriftAngular; 		/* Horizontal MRAD */
                results->table.Deriv[i][1] = solver[i].Deriv_mrad + HorizDriftAngular; 	/* Derivation MRAD */
            }

			results->table.Time[i] = solver[i].Time; 			/* Flight time */
		}
}

double aeroJmpCorrector(double MillersFGS, const struct Bullet* bullet, const struct Rifle* const rifle, 
	const struct Meteo* const meteo, const struct Options* const options) {

	if(options->AeroJump != OPTION_YES) {
		return 0.0;
	}

	//Это подброс (или опускание пули в вертикальном направлении от бокового ветра) MOA на каждые 1 mph бокового ветра
	//см. Литц стр.91
    double MOAper1MPH = 0.01*MillersFGS - 0.0024*(bullet->length/bullet->caliber) + 0.032;
    double MOAper1MPS = MOAper1MPH*_MPHtoMPS;//а это MOA на каждые 1 м/с
    double windAngle = meteo->windDir;

    if(windAngle>=0 && windAngle<=180 && rifle->twistDir==RIGHT_TWIST) { //Ветер слева и нарезы правые - пулю поднимает вверх
        return -MOAper1MPS;
    }
    else if(windAngle>180 && windAngle<360 && rifle->twistDir==RIGHT_TWIST) { //Ветер справа и нарезы правые - пулю опускает вниз
        return MOAper1MPS;
    }
    else if(windAngle>=0 && windAngle<=180 && rifle->twistDir==LEFT_TWIST) { //Ветер справа и нарезы левые - пулю опускает вниз
        return MOAper1MPS;
    }
    else if(windAngle>180 && windAngle<360 && rifle->twistDir==LEFT_TWIST) { //Ветер слева и нарезы левые - пулю поднимает вверх
        return -MOAper1MPS;
    }
    return 0.0;
}

//TODO: refactor it!
void getWindComponents(const struct Meteo* meteo, const uint16_t dist, struct OUT windPortion* wind) {

	int index = 0;

	if(dist >= (*(*(meteo)).windData)[0].currentDistance && dist < (*(*(meteo)).windData)[1].currentDistance) {     
		goto end;
	}
	else if(dist >= (*(*(meteo)).windData)[1].currentDistance && dist < (*(*(meteo)).windData)[2].currentDistance) {
		
		index = 1;
		goto end;
	}
	else if(dist >= (*(*(meteo)).windData)[2].currentDistance && dist < (*(*(meteo)).windData)[3].currentDistance) {
		
		index = 2;
		goto end;
	}
	else if(dist >= (*(*(meteo)).windData)[3].currentDistance && dist < (*(*(meteo)).windData)[4].currentDistance) {
		
		index = 3;
		goto end;
	}
	else if(dist >= (*(*(meteo)).windData)[4].currentDistance) {
		
		index = 4;
		goto end;
	}

end:
	double windSpeed = (*(*(meteo)).windData)[index].windSpeed;
	double windDir = (*(*(meteo)).windData)[index].windDir;
	double terrainDir = (*(*(meteo)).windData)[index].terrainDir;

	wind->Wx = -windSpeed*STEP_f*cos(windDir*DegToRad);

	double crossWindSpeed = -windSpeed*STEP_f*sin(windDir*DegToRad);

	wind->Wz = crossWindSpeed*cos(terrainDir*DegToRad);
	wind->Wy = crossWindSpeed*sin(terrainDir*DegToRad);
}

void initStartWindComponents(const struct Meteo* const meteo, struct windPortion* windComps) {

	if(meteo->WindType == COMPLEX_CASE) {

		getWindComponents(meteo, 0, OUT windComps);
	}
	else {
		windComps->Wx = -meteo->windSpeed * STEP_f * cos(meteo->windDir * DegToRad);
		double crossWindSpeed = -meteo->windSpeed * STEP_f * sin(meteo->windDir * DegToRad);
		windComps->Wz = crossWindSpeed * cos(meteo->terrainDir * DegToRad);
		windComps->Wy = crossWindSpeed * sin(meteo->terrainDir * DegToRad);
	}	
}

void windComponentsForComplexCase(const struct Meteo* const meteo, struct windPortion* windComps, uint16_t currentDist) {
	
	if(meteo->WindType == COMPLEX_CASE) {
		getWindComponents(meteo, currentDist, windComps);
	}
}

void defineDragInfoForCDM(uint16_t dist, double Mach, double CCF, const struct Bullet* const bullet, 
	const struct Rifle* const rifle, struct dragAndBCInfo* dragInfo, struct calibrationDistances* calibDists) {

	if(bullet->dragFunction == CDM) {

		dragInfo->CD = DragCoefficient (G7, Mach);
		double i_7 = DragCoefficientForCDM(bullet, Mach) / dragInfo->CD;
		
		const double caliberInInch = bullet->caliber * mmToInch;
		double fakeBC = calculateFakeG7BC(bullet->mass, caliberInInch, i_7);
		dragInfo->C3 = calculateC3(CCF, fakeBC);
		
		if(dist == rifle->zeroDistance) {
			dragInfo->BCzero = fakeBC;
		}
	}
	else if(bullet->dragFunction == MBCG1) {

		double BC = BCforMBCCase(bullet, Mach);

		dragInfo->CD = DragCoefficient(G1, Mach);
		dragInfo->C3 = calculateC3(CCF, BC);
		dragInfo->BCzero = BC;
	}
	else if(bullet->dragFunction == MBCG7) {

		double BC = BCforMBCCase(bullet, Mach);

		dragInfo->CD = DragCoefficient(G7, Mach);
		dragInfo->C3 = calculateC3(CCF, BC);
		dragInfo->BCzero = BC;
	}
	else {

		dragInfo->CD = DragCoefficient (bullet->dragFunction, Mach);
		dragInfo->C3 = calculateC3(CCF, bullet->BC);
		dragInfo->BCzero = bullet->BC;
	}

	if(machAtDeepTranssonic22(Mach)) {
		calibDists->DistTrans22M = dist;
	}
	else if(machAtDeepTranssonic20(Mach)) {
		calibDists->DistTrans20M = dist;
	}
	else if(machAtDeepTranssonic18(Mach)) {
		calibDists->DistTrans18M = dist;
	}
	else if(machAtDeepTranssonic16(Mach)) {
		calibDists->DistTrans16M = dist;
	}
	else if(machAtDeepTranssonic14(Mach)) {
		calibDists->DistTrans14M = dist;
	}
	else if(machAtDeepTranssonic12(Mach)) {
		calibDists->DistTrans12M = dist;
	}
	else if(machAtTranssonic(Mach)) {
		calibDists->DistTrans = dist;
	}
	else if(machAtSubsonic(Mach)) {
		calibDists->DistSubsonic = dist;
	}
	else if(machAtDeepSubsonic(Mach)) {
		calibDists->DistDeepSubsonic = dist;
	}
    else if(machAtDeepSubsonic07(Mach)) {
        calibDists->DistDeepSubsonic07M = dist;
    }
}

/* Look up table for MultiBC */
uint8_t startIndexForMBC(double Mach) {
    
#define MACH_0_5 0
#define MACH_3_0 30    
    
    if(Mach >= 3.5) {
        
        return MACH_3_0;
    }
    
    if(Mach < 0.5) {
        
        return MACH_0_5;
    }
    
    return (Mach*10) - 5;
}

double BCforMBCCase (const struct Bullet* bullet, double Mach) {

	uint8_t startIndex = startIndexForMBC(Mach);
	uint8_t endIndex = startIndex + 1;

	double startMach = (*(*(bullet)).mbcData)[startIndex].MachNumber;
	double endMach = (*(*(bullet)).mbcData)[endIndex].MachNumber;

	double startBC = (*(*(bullet)).mbcData)[startIndex].BC;
	double endBC = (*(*(bullet)).mbcData)[endIndex].BC;

	//X = f(X1)+( f(X2) - f(X1) )*(X - X1)/(X2 - X1) 
	double BC = startBC + (endBC - startBC)*(Mach - startMach)/(endMach - startMach);
	return BC;
}

#include <iostream>

void addSomeSolutionDataToSolverStruct(double KoriolisVert, double YaeroJump, double throwAngle, const struct Inputs* const inputs, 
    const struct Options* const options, const struct Bullet* const bullet, solverUnit* solver, uint64_t index, const struct Meteo* const meteo) {

	if(options->Koriolis == OPTION_YES) { 
		
		solver[index].Koriol_h = koriolisHoriz(solver[index].Dist, solver[index].Time, inputs->latitude);
	}

	solver[index].Yrz = absoluteDropToZeroing(solver[index].Yabs, solver[index].Dst_f, throwAngle)*KoriolisVert;
	solver[index].Yrt = relativeDropWithTerrainAngle(solver[index].Yrz, solver[index].Yabs, inputs->terrainAndle);

	if(options->AeroJump == OPTION_YES) {

		double MOAatDist = (solver[index].Dist / 100.0) * MOA_;

		if(meteo->WindType == SIMPLE_CASE) {

			double aeroJumpCorr = (YaeroJump * fabs(solver[index].windSpeed) * MOAatDist);
			
			solver[index].Yrt = solver[index].Yrt + aeroJumpCorr;
		}
		else {

			int complexWindIndex = 0;

			if(inputs->shotDistance >= (*(*(meteo)).windData)[0].currentDistance && inputs->shotDistance < (*(*(meteo)).windData)[1].currentDistance) {     
				complexWindIndex = 0;
			}
			else if(inputs->shotDistance >= (*(*(meteo)).windData)[1].currentDistance && inputs->shotDistance < (*(*(meteo)).windData)[2].currentDistance) {
				complexWindIndex = 1;
			}
			else if(inputs->shotDistance >= (*(*(meteo)).windData)[2].currentDistance && inputs->shotDistance < (*(*(meteo)).windData)[3].currentDistance) {
				complexWindIndex = 2;
			}
			else if(inputs->shotDistance >= (*(*(meteo)).windData)[3].currentDistance && inputs->shotDistance < (*(*(meteo)).windData)[4].currentDistance) {
				complexWindIndex = 3;
			}
			else if(inputs->shotDistance >= (*(*(meteo)).windData)[4].currentDistance) {
				complexWindIndex = 4;
			}

			double averageWindSpeed = 0;
			double windSpeed = 0;
			uint16_t windDir = 0;

			for(uint8_t i = 0; i <= complexWindIndex; i++) {
				
				windSpeed = (*(*(meteo)).windData)[i].windSpeed;
				windDir = (*(*(meteo)).windData)[i].windDir;

				averageWindSpeed += windSpeed * sin(DegToRad*windDir);	
			}

			averageWindSpeed /= (complexWindIndex + 1);

			double aeroJumpCorr = (YaeroJump * averageWindSpeed * MOAatDist);
			
			solver[index].Yrt = solver[index].Yrt + aeroJumpCorr;
		}
	}

	solver[index].Yrt_moa = convertSmToMOA(solver[index].Yrt, solver[index].Dist);
	solver[index].Yrt_mrad = convertMOAToMRAD(solver[index].Yrt_moa);
	solver[index].W += solver[index].Koriol_h;
	solver[index].W_moa = convertSmToMOA(solver[index].W,solver[index].Dist);
	solver[index].W_mrad = convertMOAToMRAD(solver[index].W_moa);			
	solver[index].Deriv_moa = convertSmToMOA(solver[index].Deriv,solver[index].Dist);
	solver[index].Deriv_mrad = convertMOAToMRAD(solver[index].Deriv_moa);

	if(bullet->dragFunction != CDM) {
		solver[index].Yrt *= linearInterpolationDSF(bullet, solver[index].MachNumber);
	}		
}

double calculateC3(double CCF, double BC) {
	return (-0.0002048757 * CCF) / BC;
}


int32_t calculateAbsDrop(double Yabs) {
	return -convertFromFeets(Yabs) * 100.0;
}

double convertToFeets(double value) {
	return value * STEP_f;
}

double convertFromFeets(double value) {
	return value / STEP_f;
}

///////////////////////////////////////////////////////////////////////////////////////////////

double DragCoefficient (uint8_t DragFunction, double M) {
			
	double Cd = 0.0;
	
	if (DragFunction == G7) {

#ifndef SLOW_DEVICE
		/* Most Precise G7 approximation */
		if (M<=0.749) {Cd=0.0012*M+0.1192;}
		else if (M<=0.949) {Cd=114.6667*pow(M,4)-369.2162*pow(M,3)+446.1956*pow(M,2)-239.7609*M+48.4391;}
		else if (M<=0.9749) {Cd=43.6*pow(M,2)-80.174*M+37.0217;}
		else if (M<=1.049) {Cd=441.6*pow(M,3)-1372.64*pow(M,2)+1422.248*M-490.8277;}
		else if (M<=2.049) {Cd=0.1185*pow(M,5)-1.1073*pow(M,4)+4.0521*pow(M,3)-7.1962*pow(M,2)+6.05*M-1.5097;}
		else if (M<=4.0) {Cd=-0.0547*M+0.4064;}
#endif
#if defined SLOW_DEVICE
		/* Linear G7 approximation (variant 2)*/

		if (M<=0.8) {Cd=0.120;}
		else if (M<=0.90) {Cd=0.264*M-0.0912;}  
		else if (M<=0.95) {Cd=1.18*M-0.9156;}  
		else if (M<=1.0) {Cd=3.498*M-3.1177;}  
		else if (M<=1.025) {Cd=0.848*M-0.4677;}
		else if (M<=1.1) {Cd=0.403;}
		else if (M>1.1) {Cd=0.4228/(sqrt(M));}
#endif
	}

	else if (DragFunction == G1) {
	
#ifndef SLOW_DEVICE
		/* Most Precise G1 approximation */
		if (M<=0.499) {Cd=0.0852*pow(M,2)-0.1657*M+0.2637;}
		else if (M<=0.999) {Cd=-89.1534*pow(M,6)+370.3571*pow(M,5)-630.2645*pow(M,4)+564.7044*pow(M,3)-281.0384*pow(M,2)+73.5809*M-7.705;} 
		else if (M<=1.499) {Cd=-3.8058*pow(M,4)+21.5685*pow(M,3)-46.2897*pow(M,2)+44.5479*M-15.54;}
		else if (M<=2.499) {Cd=0.067212*pow(M,3)-0.38359*pow(M,2)+0.593708*M+0.403095;}
		else if (M<=4.0) {Cd=0.0459*pow(M,2)-0.3051*M+1.0156;}
#endif
#if defined SLOW_DEVICE
 		/* Linear G1 approximation (variant 2)*/

		if (M<=0.7) {Cd=0.208;}
		else if (M<=0.825) {Cd=0.4328*M -0.0865;}
		else if (M<=1.1) {Cd= 1.1553*M-0.6825;}
		else if (M<=1.25) {Cd=0.478*M+0.0625;}
		else if (M<=1.5) {Cd=0.66;}
		else if (M>1.5) {Cd=0.7712*pow(M,-0.384);}
#endif
	}

	else if (DragFunction == Gs) {

		/* Only linear approximation exists at the moment for G-sphere*/
		if (M<=0.55) {Cd=0.0551*M+0.4662;}
		else if (M<=1.15) {Cd=0.7301*M+0.0819;}
		else if (M<=1.3) {Cd=0.522*M+0.3184;}
		else if (M<=1.6) {Cd=0.0564*M+0.92175;}
		else if (M>1.6) {Cd=-0.03698*M+1.07324;}
	}

	return Cd; 
}