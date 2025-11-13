#include "roll_angle.h"

void resetAngles(struct rifleAngles* angles) {

	memset(angles, 0, sizeof(rifleAngles));
}

void correctViaRollAngle(struct rifleAngles* angles, int16_t rollAngle) {

	/* Угол положительный по часовой стрелке, отрицательный - против (если смотреть на цель) */

	const double degsToRads = 0.017453;
	const double rollInRads = -(rollAngle * degsToRads);

	const double X = angles->horizInit;
	const double Y = angles->vertInit;

	angles->vertCorr = -X * sin(rollInRads) + Y * cos(rollInRads);
	angles->horizCorr = X * cos(rollInRads) + Y * sin(rollInRads);
}