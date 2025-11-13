#include <iostream>
#include  "profiles_config_worker.h"

/*

////////////////////////////////////////////////////////////////////////

void checkWrite(bool status) {

	std::cout << "Write is: " << std::boolalpha << status << std::endl;
}


void checkRead(bool status) {

	std::cout << "Read is: " << std::boolalpha << status << std::endl;
}

*/

////////////////////////////////////////////////////////////////////////

int main () {

	UserProfilesWorker upw;
	upw.loadFromFile();

	return 0;
}