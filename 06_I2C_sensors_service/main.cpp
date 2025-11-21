#include <unistd.h>
#include <string>
#include <iostream>

#include "I2C_daemon.h"

int main(int argc, char* argv[]) {

/*
    if(argc < 2) {

        std::cerr << "Не задан путь к файлу конфига. Использование:" << std::endl;
        std::cerr << argv[0] << " <путь к конфигу *.ini>" << std::endl;
        return 1;
    }

	i2cDaemon i2cDi{std::string(argv[1]), std::string(argv[0])};
*/
	i2cDaemon i2cDi{"conf.ini", "i2c_daemon"};

	if(!i2cDi.init()) {
		return 2;
	}

	i2cDi.run();
	return 0;
}