#include <iostream>
#include "ballistics_config_worker.h"

////////////////////////////////////////////////////////////////////////

void checkWrite(bool status) {

	std::cout << "Write is: " << std::boolalpha << status << std::endl;
}


void checkRead(bool status) {

	std::cout << "Read is: " << std::boolalpha << status << std::endl;
}

////////////////////////////////////////////////////////////////////////

int main () {

	configWorker cw;

	cw.setConfigWriteCallback(checkWrite);
	cw.setConfigReadCallback(checkRead);
	cw.readConfig();

	auto bulletsArray = cw.getBulletsInfo();
	auto bullet = cw.getCurrentBulletInfo();

	auto rifleArray = cw.getRiflesInfo();
	auto rifle = cw.getCurrentRifleInfo();

	auto settings = cw.getDeviceSettings();
	auto inputs = cw.getBCInputs();
	auto target = cw.getTargetData();
	auto mildot = cw.getMildotInputs();

	return 0;
}