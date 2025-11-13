#ifndef _ROLL_ANGLE_H_
#define _ROLL_ANGLE_H_

#include <string.h>
#include <math.h>
#include <stdint.h>

struct rifleAngles {

	double vertInit;
	double horizInit;
	double vertCorr;
	double horizCorr;
};

struct roolCorrectionData {

	double MOAcorrection;
	double MRADcorrection;
};

void resetAngles(struct rifleAngles* angles);
void correctViaRollAngle(struct rifleAngles* angles, int16_t rollAngle);

#endif /* _ROLL_ANGLE_H_ */