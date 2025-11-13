#include <iostream>
#include "peripherals_settings_worker.h"

void checkStatus(bool status) {

	std::cout << "Write is: " << std::boolalpha << status << std::endl;
}


void checkRead(bool status) {

	std::cout << "Read is: " << std::boolalpha << status << std::endl;
}
 
int main () {

	PeripheralSettingsWorker psw{};
	psw.setConfigWriteCallback(checkStatus);
	psw.setConfigReadCallback(checkRead);
	psw.loadFromFile();

	dev_setting_t prev;
	prev = psw.getSettings();
	psw.getSettings().compass.active = !psw.getSettings().compass.active;
	std::cout << std::boolalpha << (prev != psw.getSettings()) << std::endl;

	return 0;
}